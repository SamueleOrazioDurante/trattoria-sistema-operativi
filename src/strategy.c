#include "strategy.h"
#include <stddef.h>

// Helper per determinare se serve una rotazione in base a stanchezza e resistenza
static tr_bool_t needs_rotation(int staff_id, const snapshot_t *snapshot, const staff_member_t *staff_info) {
    level_t fatigue = snapshot->staff_fatigue[staff_id];
    param_bucket_t resistance = staff_info[staff_id].traits[TRAIT_RESISTANCE];

    if (fatigue == LVL_HIGH) {
        // Rotazione forzata se stanchezza alta
        return TR_TRUE;
    } else if (fatigue == LVL_MED) {
        // Rotazione preventiva se la stanchezza è media, ma considerare la resistenza
        // Se la resistenza è alta, possiamo resistere ancora un po'. Altrimenti ruotiamo.
        if (resistance == PARAM_LOW || resistance == PARAM_MEDIUM) {
            return TR_TRUE;
        }
    }
    return TR_FALSE;
}

static role_t strategy_profit(int staff_id, const snapshot_t *snapshot, const staff_member_t *staff_info, int staff_n) {
    
    if (needs_rotation(staff_id, snapshot, staff_info)) {
        if (snapshot->staff_fatigue[staff_id] == LVL_HIGH) {
            return ROLE_NONE; // Riposo
        }
    }

    const staff_member_t *me = &staff_info[staff_id];

    int pending_orders = snapshot->kitchen->pending_orders;
    int tables_waiting_order = 0;
    int tables_dirty = 0;
    int pending_payments = snapshot->cashdesk->pending_payments;

    // Logica di osservazione delle famiglie
    int estimated_cook_load = pending_orders;
    int estimated_clean_load = 0;

    for (int i = 0; i < snapshot->diningroom->tables_n; i++) {
        table_state_t st = snapshot->diningroom->tables[i].state;
        if (st == TABLE_TAKEN) {
            tables_waiting_order++;
            // food_qty visibile dopo l'ordine, stimiamo il carico per la cucina
            if (snapshot->diningroom->tables[i].food_qty != LVL_NONE) {
                estimated_cook_load += snapshot->diningroom->tables[i].food_qty;
            }
        } else if (st == TABLE_FREED) {
            tables_dirty++;
            // dirt_level visibile dopo il consumo, pianifichiamo pulizia
            if (snapshot->diningroom->tables[i].dirt_level != LVL_NONE) {
                estimated_clean_load += snapshot->diningroom->tables[i].dirt_level;
            }
        }
    }

    // 1. Cuoco sempre attivo se ci sono ordini pendenti
    if (estimated_cook_load > 0 && (snapshot->blackboard->cook == -1 || snapshot->blackboard->cook == staff_id)) {
        // Privilegiare chi ha abilità ALTA nel ruolo
        if (me->skills[SKILL_COOK] == PARAM_HIGH || snapshot->blackboard->cook == staff_id) {
            return ROLE_COOK;
        } else if (me->skills[SKILL_COOK] == PARAM_MEDIUM) {
            return ROLE_COOK;
        }
    }

    // 2. Assegnare subito pulitori/camerieri ai tavoli in attesa (Priorità alla velocità)
    if (tables_waiting_order > 0) {
        if (me->skills[SKILL_WAITER] >= PARAM_MEDIUM) {
            return ROLE_WAITER;
        }
    }

    if (tables_dirty > 0 || estimated_clean_load > 0) {
        if (me->skills[SKILL_HELPER] >= PARAM_MEDIUM) {
            return ROLE_HELPER;
        }
    }

    // 3. Minimizzare i tempi morti assegnando altri ruoli se necessario
    if (pending_payments > 0 && (snapshot->blackboard->cashier == -1 || snapshot->blackboard->cashier == staff_id)) {
        return ROLE_CASHIER;
    }

    // Se ci sono ordini pendenti e nessun cuoco con alta abilità ha preso il ruolo, 
    // prendiamolo per evitare stalli e minimizzare i tempi (fallback)
    if (estimated_cook_load > 0 && snapshot->blackboard->cook == -1) {
        return ROLE_COOK;
    }
    
    // Fallback per minimizzare tempi morti anche con bassa abilità
    if (tables_waiting_order > 0) return ROLE_WAITER;
    if (tables_dirty > 0) return ROLE_HELPER;

    return ROLE_NONE;
}

static role_t strategy_reputation(int staff_id, const snapshot_t *snapshot, const staff_member_t *staff_info, int staff_n) {
    
    if (needs_rotation(staff_id, snapshot, staff_info)) {
        if (snapshot->staff_fatigue[staff_id] == LVL_HIGH) {
            return ROLE_NONE; // Riposo
        }
    }

    const staff_member_t *me = &staff_info[staff_id];

    int pending_orders = snapshot->kitchen->pending_orders;
    int tables_waiting_order = 0;
    int tables_dirty = 0;
    int pending_payments = snapshot->cashdesk->pending_payments;

    for (int i = 0; i < snapshot->diningroom->tables_n; i++) {
        table_state_t st = snapshot->diningroom->tables[i].state;
        if (st == TABLE_TAKEN) {
            tables_waiting_order++;
        } else if (st == TABLE_FREED) {
            tables_dirty++;
        }
    }

    // Identificare personale altamente qualificato per il contatto col pubblico
    tr_bool_t good_contact = (me->traits[TRAIT_SOCIABILITY] == PARAM_HIGH || 
                              me->traits[TRAIT_PROFESSIONALITY] == PARAM_HIGH || 
                              me->traits[TRAIT_PATIENCE] == PARAM_HIGH);

    // 1. Assegnare personale con pazienza/socievolezza/professionalità alta ai ruoli di contatto
    if (good_contact) {
        if (tables_waiting_order > 0) {
            return ROLE_WAITER;
        }
        if (pending_payments > 0 && (snapshot->blackboard->cashier == -1 || snapshot->blackboard->cashier == staff_id)) {
            return ROLE_CASHIER;
        }
    }

    // 2. Personale meno orientato al contatto o ruoli di background
    if (pending_orders > 0 && (snapshot->blackboard->cook == -1 || snapshot->blackboard->cook == staff_id)) {
        if (me->skills[SKILL_COOK] >= PARAM_MEDIUM) {
            return ROLE_COOK;
        }
    }

    if (tables_dirty > 0) {
        return ROLE_HELPER;
    }

    // 3. Bilanciare qualità del servizio vs velocità:
    // "Accettare tempi maggiori in favore di personale più qualificato"
    // Questo significa che se non sono molto socievole, evito di fare il cameriere anche se c'è attesa,
    // a meno che la cucina sia completamente bloccata o simili emergenze.
    
    // Se c'è urgenza in cucina e non c'è nessuno
    if (pending_orders > 0 && snapshot->blackboard->cook == -1) {
        return ROLE_COOK; 
    }

    // Se sono bravo nel contatto ma non ci sono clienti, aiuto con la pulizia
    if (good_contact && tables_waiting_order == 0 && pending_payments == 0 && tables_dirty > 0) {
        return ROLE_HELPER;
    }

    return ROLE_NONE;
}

role_t strategy_decide_role(int staff_id, strategy_t strategy, const snapshot_t *snapshot, const staff_member_t *staff_info, int staff_n) {
    if (strategy == STRATEGY_PROFIT) {
        return strategy_profit(staff_id, snapshot, staff_info, staff_n);
    } else if (strategy == STRATEGY_REPUTATION) {
        return strategy_reputation(staff_id, snapshot, staff_info, staff_n);
    }
    return ROLE_NONE;
}
