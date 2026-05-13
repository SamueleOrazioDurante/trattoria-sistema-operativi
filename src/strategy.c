#include "strategy.h"
#include <stddef.h>
#include <stdio.h>

/* ========================================================================= */
/*  HELPER                                                                   */
/* ========================================================================= */

// Determina se serve una rotazione in base a stanchezza e resistenza
static tr_bool_t needs_rotation(int staff_id, const snapshot_t *snapshot, const staff_member_t *staff_info) {
    level_t fatigue = snapshot->staff_fatigue[staff_id];
    param_bucket_t resistance = staff_info[staff_id].traits[TRAIT_RESISTANCE];

    if (fatigue == LVL_HIGH) return TR_TRUE;
    if (fatigue == LVL_MED && (resistance == PARAM_LOW || resistance == PARAM_MEDIUM)) return TR_TRUE;
    return TR_FALSE;
}

// Controlla se un ruolo comune è già coperto da qualcun altro
static tr_bool_t common_role_taken_by_other(int staff_id, int assigned_id) {
    return (assigned_id != -1 && assigned_id != staff_id) ? TR_TRUE : TR_FALSE;
}

// La cucina è completamente bloccata (zero piatti puliti)
static tr_bool_t kitchen_blocked(const snapshot_t *snapshot) {
    return (snapshot->kitchen->clean_plates == LVL_NONE) ? TR_TRUE : TR_FALSE;
}

// La cucina è a corto di piatti
static tr_bool_t kitchen_low_on_plates(const snapshot_t *snapshot) {
    return (snapshot->kitchen->clean_plates <= LVL_LOW || snapshot->kitchen->dirty_plates >= LVL_MED) ? TR_TRUE : TR_FALSE;
}

// Controlla se c'è della preparazione in corso per qualche tavolo
static tr_bool_t cooking_in_progress(const snapshot_t *snapshot) {
    for (int i = 0; i < snapshot->diningroom->tables_n; i++) {
        if (snapshot->diningroom->tables[i].state == TABLE_TAKEN &&
            snapshot->diningroom->tables[i].food_qty != LVL_NONE &&
            snapshot->kitchen->food_ready[i] == TR_FALSE) {
            return TR_TRUE;
        }
    }
    return TR_FALSE;
}

// Conta i tavoli che hanno il cibo pronto da servire
static int count_food_ready(const snapshot_t *snapshot) {
    int count = 0;
    for (int i = 0; i < snapshot->diningroom->tables_n; i++) {
        if (snapshot->kitchen->food_ready[i] == TR_TRUE &&
            snapshot->blackboard->tables[i].waiter == -1) {
            count++;
        }
    }
    return count;
}

/**
 * Persistenza del ruolo: Controlla se dobbiamo rimanere nel nostro ruolo comune attuale.
 * Rimaniamo solo se c'è lavoro attivo e NON c'è estrema urgenza altrove.
 */
static role_t check_persistence(int staff_id, const snapshot_t *snapshot, tr_bool_t extreme_urgency) {
    if (extreme_urgency) return ROLE_NONE;

    // Rimani come cuoco se stai ancora cucinando
    if (snapshot->blackboard->cook == staff_id) {
        if (snapshot->kitchen->pending_orders > 0 || cooking_in_progress(snapshot)) {
            return ROLE_COOK;
        }
    }

    // Rimani come cassiere se ci sono persone in attesa
    if (snapshot->blackboard->cashier == staff_id) {
        if (snapshot->cashdesk->pending_payments > 0) {
            return ROLE_CASHIER;
        }
    }

    // Rimani come lavapiatti se c'è molto da lavare
    if (snapshot->blackboard->dishwasher == staff_id) {
        if (snapshot->kitchen->dirty_plates >= LVL_MED) {
            return ROLE_DISHWASHER;
        }
    }

    return ROLE_NONE;
}

/* ========================================================================= */
/*  STRATEGIA: PROFIT                                                        */
/* ========================================================================= */

static role_t strategy_profit(int staff_id, const snapshot_t *snapshot, const staff_member_t *staff_info, int staff_n) {
    if (needs_rotation(staff_id, snapshot, staff_info)) {
        if (snapshot->staff_fatigue[staff_id] == LVL_HIGH) return ROLE_NONE;
    }

    int pending_payments = snapshot->cashdesk->pending_payments;
    int food_ready_count = count_food_ready(snapshot);
    tr_bool_t blocked = kitchen_blocked(snapshot);

    // Persistenza a meno che qualcuno non debba servire cibo o pagare urgentemente, o la cucina sia bloccata
    role_t persistent = check_persistence(staff_id, snapshot, (food_ready_count > 0 || pending_payments > 2 || blocked));
    if (persistent != ROLE_NONE) return persistent;

    const staff_member_t *me = &staff_info[staff_id];
    int pending_orders = snapshot->kitchen->pending_orders;
    int tables_waiting_order = 0;
    int tables_dirty = 0;
    
    for (int i = 0; i < snapshot->diningroom->tables_n; i++) {
        table_state_t st = snapshot->diningroom->tables[i].state;
        if (st == TABLE_TAKEN && snapshot->diningroom->tables[i].food_qty == LVL_NONE) tables_waiting_order++;
        else if (st == TABLE_FREED && snapshot->blackboard->tables[i].cleaner == -1) tables_dirty++;
    }

    // 0. LAVAPIATTI se bloccato
    if (blocked && !common_role_taken_by_other(staff_id, snapshot->blackboard->dishwasher)) return ROLE_DISHWASHER;

    // 1. SERVIRE CIBO
    if (food_ready_count > 0) return ROLE_WAITER;

    // 2. CUOCO (Critico per il progresso)
    if ((pending_orders > 0 || cooking_in_progress(snapshot)) && !common_role_taken_by_other(staff_id, snapshot->blackboard->cook)) {
        if (me->skills[SKILL_COOK] >= PARAM_MEDIUM || snapshot->blackboard->cook == -1) return ROLE_COOK;
    }

    // 3. CASSIERE
    if (pending_payments > 0 && !common_role_taken_by_other(staff_id, snapshot->blackboard->cashier)) return ROLE_CASHIER;

    // 4. PRENDERE ORDINI
    if (tables_waiting_order > 0) return ROLE_WAITER;

    // 5. PULIRE TAVOLI
    if (tables_dirty > 0) return ROLE_HELPER;

    // 6. LAVAPIATTI PREVENTIVO
    if (kitchen_low_on_plates(snapshot) && snapshot->blackboard->dishwasher == -1) return ROLE_DISHWASHER;

    // Fallback
    if (pending_orders > 0 && snapshot->blackboard->cook == -1) return ROLE_COOK;
    if (pending_payments > 0 && snapshot->blackboard->cashier == -1) return ROLE_CASHIER;

    return ROLE_NONE;
}

/* ========================================================================= */
/*  STRATEGIA: REPUTATION                                                    */
/* ========================================================================= */

static role_t strategy_reputation(int staff_id, const snapshot_t *snapshot, const staff_member_t *staff_info, int staff_n) {
    if (needs_rotation(staff_id, snapshot, staff_info)) {
        if (snapshot->staff_fatigue[staff_id] == LVL_HIGH) return ROLE_NONE;
    }

    const staff_member_t *me = &staff_info[staff_id];
    tr_bool_t good_contact = (me->traits[TRAIT_SOCIABILITY] == PARAM_HIGH ||
                              me->traits[TRAIT_PROFESSIONALITY] == PARAM_HIGH ||
                              me->traits[TRAIT_PATIENCE] == PARAM_HIGH);

    int pending_payments = snapshot->cashdesk->pending_payments;
    int food_ready_count = count_food_ready(snapshot);
    tr_bool_t blocked = kitchen_blocked(snapshot);

    // Logica di persistenza (non persistere se siamo good contact e qualcuno deve pagare/ordinare, o se bloccato)
    role_t persistent = check_persistence(staff_id, snapshot, (good_contact && (pending_payments > 0 || food_ready_count > 0)) || blocked);
    if (persistent != ROLE_NONE) return persistent;

    int pending_orders = snapshot->kitchen->pending_orders;
    int tables_waiting_order = 0;
    int tables_dirty = 0;
    
    for (int i = 0; i < snapshot->diningroom->tables_n; i++) {
        table_state_t st = snapshot->diningroom->tables[i].state;
        if (st == TABLE_TAKEN && snapshot->diningroom->tables[i].food_qty == LVL_NONE) tables_waiting_order++;
        else if (st == TABLE_FREED && snapshot->blackboard->tables[i].cleaner == -1) tables_dirty++;
    }

    // 0. LAVAPIATTI se bloccato
    if (blocked && !common_role_taken_by_other(staff_id, snapshot->blackboard->dishwasher)) {
        if (!good_contact || snapshot->blackboard->dishwasher == -1) return ROLE_DISHWASHER;
    }

    // 1. SERVIRE CIBO (Tutti lo fanno, il cibo freddo è male)
    if (food_ready_count > 0) return ROLE_WAITER;

    // 2. CUOCO
    if ((pending_orders > 0 || cooking_in_progress(snapshot)) && !common_role_taken_by_other(staff_id, snapshot->blackboard->cook)) {
        if (me->skills[SKILL_COOK] >= PARAM_MEDIUM || snapshot->blackboard->cook == -1) return ROLE_COOK;
    }

    // 3. CASSIERE (Preferito good contact)
    if (pending_payments > 0 && !common_role_taken_by_other(staff_id, snapshot->blackboard->cashier)) {
        if (good_contact || snapshot->blackboard->cashier == -1) return ROLE_CASHIER;
    }

    // 4. PRENDERE ORDINI (Preferito good contact)
    if (tables_waiting_order > 0) {
        if (good_contact) return ROLE_WAITER;
    }

    // 5. PULIRE TAVOLI
    if (tables_dirty > 0) return ROLE_HELPER;

    // 6. LAVAPIATTI PREVENTIVO (Preferito non-contact)
    if (kitchen_low_on_plates(snapshot) && snapshot->blackboard->dishwasher == -1) {
        if (!good_contact) return ROLE_DISHWASHER;
    }

    // Fallback
    if (tables_waiting_order > 0) return ROLE_WAITER;
    if (pending_payments > 0 && snapshot->blackboard->cashier == -1) return ROLE_CASHIER;
    if (pending_orders > 0 && snapshot->blackboard->cook == -1) return ROLE_COOK;
    if (kitchen_low_on_plates(snapshot) && snapshot->blackboard->dishwasher == -1) return ROLE_DISHWASHER;

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
