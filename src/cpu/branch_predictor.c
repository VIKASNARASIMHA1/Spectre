#include "cpu.h"

BranchPredictor* bp_create(PredictorType type, int bhr_size, int pht_size) {
    BranchPredictor* bp = (BranchPredictor*)malloc(sizeof(BranchPredictor));
    if (!bp) return NULL;
    
    bp->type = type;
    bp->bhr_size = bhr_size;
    bp->pht_size = pht_size;
    bp->bhr = 0;
    bp->correct = 0;
    bp->total = 0;
    
    bp->pht = (uint8_t*)malloc(pht_size * sizeof(uint8_t));
    if (!bp->pht) {
        free(bp);
        return NULL;
    }
    
    // Initialize PHT entries to weakly taken (2)
    for (int i = 0; i < pht_size; i++) {
        bp->pht[i] = 2;
    }
    
    return bp;
}

void bp_destroy(BranchPredictor* bp) {
    if (bp) {
        free(bp->pht);
        free(bp);
    }
}

bool bp_predict(BranchPredictor* bp, uint64_t pc) {
    bp->total++;
    
    switch (bp->type) {
        case PREDICTOR_ALWAYS_TAKEN:
            return true;
            
        case PREDICTOR_ALWAYS_NOT_TAKEN:
            return false;
            
        case PREDICTOR_BIMODAL: {
            uint32_t index = pc % bp->pht_size;
            return bp->pht[index] >= 2;
        }
            
        case PREDICTOR_GSHARE: {
            uint32_t index = (pc ^ bp->bhr) % bp->pht_size;
            return bp->pht[index] >= 2;
        }
            
        default:
            return false;
    }
}

void bp_update(BranchPredictor* bp, uint64_t pc, bool taken, bool predicted) {
    if (taken == predicted) {
        bp->correct++;
    }
    
    switch (bp->type) {
        case PREDICTOR_BIMODAL: {
            uint32_t index = pc % bp->pht_size;
            if (taken) {
                if (bp->pht[index] < 3) bp->pht[index]++;
            } else {
                if (bp->pht[index] > 0) bp->pht[index]--;
            }
            break;
        }
            
        case PREDICTOR_GSHARE: {
            uint32_t index = (pc ^ bp->bhr) % bp->pht_size;
            if (taken) {
                if (bp->pht[index] < 3) bp->pht[index]++;
            } else {
                if (bp->pht[index] > 0) bp->pht[index]--;
            }
            
            // Update branch history register
            bp->bhr = (bp->bhr << 1) | (taken ? 1 : 0);
            bp->bhr &= (1 << bp->bhr_size) - 1;
            break;
        }
            
        default:
            break;
    }
}

void bp_print_stats(BranchPredictor* bp) {
    printf("\n=== Branch Predictor Stats ===\n");
    printf("Type: %d\n", bp->type);
    printf("Total predictions: %lu\n", bp->total);
    printf("Correct predictions: %lu\n", bp->correct);
    printf("Accuracy: %.2f%%\n", 
           bp->total > 0 ? (100.0 * bp->correct / bp->total) : 0.0);
}