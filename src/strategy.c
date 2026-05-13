#include "strategy.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_STAFF 4 // Assumo massimo 4 staff come da traccia

static int cashier_turns[MAX_STAFF] = {0};

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
    // Solo HIGH fatigue = riposo. A MED continuiamo a lavorare per velocità.
    if (snapshot->staff_fatigue[staff_id] == LVL_HIGH) return ROLE_NONE;

    int pending_payments = snapshot->cashdesk->pending_payments;
    int food_ready_count = count_food_ready(snapshot);
    tr_bool_t blocked = kitchen_blocked(snapshot);
    int pending_orders = snapshot->kitchen->pending_orders;

    int tables_waiting_order = 0;
    int tables_dirty = 0;
    int families_paying = 0;
    
    for (int i = 0; i < snapshot->diningroom->tables_n; i++) {
        table_state_t st = snapshot->diningroom->tables[i].state;
        if (st == TABLE_TAKEN && snapshot->diningroom->tables[i].food_qty == LVL_NONE) tables_waiting_order++;
        else if (st == TABLE_FREED && snapshot->blackboard->tables[i].cleaner == -1) tables_dirty++;
        if (st == TABLE_FREED) families_paying++;
    }

    // 0. LAVAPIATTI se cucina bloccata (CRITICO - blocca tutto il pipeline)
    if (blocked && !common_role_taken_by_other(staff_id, snapshot->blackboard->dishwasher)) return ROLE_DISHWASHER;

    // 1. SERVIRE CIBO (sblocca tavolo → famiglia mangia → paga → se ne va)
    if (food_ready_count > 0) return ROLE_WAITER;

    // 2. LAVAPIATTI PREVENTIVO prima che si blocchi la cucina
    if (snapshot->kitchen->clean_plates < 2 && 
        snapshot->kitchen->dirty_plates > 0 &&
        !common_role_taken_by_other(staff_id, snapshot->blackboard->dishwasher)) {
        return ROLE_DISHWASHER;
    }

    // 3. CUOCO - il cibo deve essere pronto il prima possibile
    if ((pending_orders > 0 || cooking_in_progress(snapshot)) && 
        !common_role_taken_by_other(staff_id, snapshot->blackboard->cook)) {
        return ROLE_COOK;
    }

    // 4. CASSIERE - processare pagamenti subito (nessun cooldown!)
    if ((pending_payments > 0 || families_paying > 0) && 
        !common_role_taken_by_other(staff_id, snapshot->blackboard->cashier)) {
        return ROLE_CASHIER;
    }

    // 5. PRENDERE ORDINI
    if (tables_waiting_order > 0) return ROLE_WAITER;

    // 6. PULIRE TAVOLI (necessario per far entrare nuove famiglie)
    if (tables_dirty > 0) return ROLE_HELPER;

    // 7. LAVAPIATTI PREVENTIVO (meno urgente)
    if (kitchen_low_on_plates(snapshot) && snapshot->blackboard->dishwasher == -1) return ROLE_DISHWASHER;

    // Fallback
    if (families_paying > 0 && snapshot->blackboard->cashier == -1) return ROLE_CASHIER;
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

    int pending_payments = snapshot->cashdesk->pending_payments;
    int food_ready_count = count_food_ready(snapshot);
    int pending_orders = snapshot->kitchen->pending_orders;
    
    int families_paying = 0;
    for (int i = 0; i < snapshot->diningroom->tables_n; i++) {
        if (snapshot->diningroom->tables[i].state == TABLE_FREED) families_paying++;
    }

    int tables_waiting_order = 0;
    int tables_dirty = 0;
    for (int i = 0; i < snapshot->diningroom->tables_n; i++) {
        table_state_t st = snapshot->diningroom->tables[i].state;
        if (st == TABLE_TAKEN && snapshot->diningroom->tables[i].food_qty == LVL_NONE) tables_waiting_order++;
        else if (st == TABLE_FREED && snapshot->blackboard->tables[i].cleaner == -1) tables_dirty++;
    }

    // Giulia (ID 0): Cuoco preferito (Skill MED)
    if (staff_id == 0) {
        if (pending_orders > 0 || cooking_in_progress(snapshot)) return ROLE_COOK;
        if (snapshot->kitchen->dirty_plates > 0 && snapshot->kitchen->clean_plates < 3) return ROLE_DISHWASHER;
        if (tables_dirty > 0) return ROLE_HELPER;
        return ROLE_NONE;
    }

    // Matteo (ID 1): Cassiere preferito (Skill HIGH)
    if (staff_id == 1) {
        if (pending_payments > 0 || families_paying > 0) return ROLE_CASHIER;
        if (snapshot->kitchen->dirty_plates > 0 && snapshot->kitchen->clean_plates < 3) return ROLE_DISHWASHER;
        if (tables_dirty > 0) return ROLE_HELPER;
        return ROLE_NONE;
    }

    // Gabriele (ID 2): Cameriere preferito (Skill HIGH, Pazienza HIGH)
    if (staff_id == 2) {
        if (food_ready_count > 0) return ROLE_WAITER;
        if (tables_waiting_order > 0) return ROLE_WAITER;
        if (tables_dirty > 0) return ROLE_HELPER;
        return ROLE_NONE;
    }

    // Beatrice (ID 3): Cameriere preferito (Skill HIGH, Pazienza HIGH)
    if (staff_id == 3) {
        if (food_ready_count > 0) return ROLE_WAITER;
        if (tables_waiting_order > 0) return ROLE_WAITER;
        if (tables_dirty > 0) return ROLE_HELPER;
        return ROLE_NONE;
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
