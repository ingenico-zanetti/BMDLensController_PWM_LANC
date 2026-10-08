#ifndef __MEDIAN_FILTER_HPP_INCLUDED__
#define __MEDIAN_FILTER_HPP_INCLUDED__

//  #include <stdint.h>

#include "Arduino.h"

template <uint8_t N>
class MedianFilter {
    static_assert(N > 2, "La taille de la fenêtre doit être supérieure à 2.");
    static_assert(N % 2 != 0, "La taille de la fenêtre doit être impaire.");

public:
    static constexpr uint8_t MEDIAN_IDX = N / 2;

    explicit MedianFilter(const char *name, uint16_t init_value = 0):szName(name) {
        preload(init_value);
    }

    /**
     * @brief Préremplit la fenêtre glissante avec une valeur unique.
     * Complexité O(N) : aucun tri nécessaire car toutes les valeurs sont identiques.
     */
    void preload(uint16_t init_value = 0) {
        head = 0;
        for (uint8_t i = 0; i < N; ++i) {
            history[i] = init_value;
            sorted_list[i] = init_value;
            pos_in_sorted[i] = i;
            sorted_to_fifo[i] = i;
        }
    }

    /**
     * @brief Traitement d'un échantillon en O(N) déterministe (sans branchement conditionnel).
     */
    uint16_t update(uint16_t new_val) {
        // 1. Indice dans sorted_list de l'échantillon le plus ancien
        const uint8_t fifo_idx = head;
        uint8_t sorted_idx = pos_in_sorted[fifo_idx];

        // 2. Mémorisation du nouvel échantillon dans le buffer FIFO circulaire
        history[fifo_idx] = new_val;
        head = (fifo_idx + 1) % N;

        // 3. Remplacement direct dans la liste triée
        sorted_list[sorted_idx] = new_val;

        // 4. Maintien de l'ordre par décalages simples
        // Vers la droite
        while (sorted_idx < N - 1 && sorted_list[sorted_idx] > sorted_list[sorted_idx + 1]) {
            swap_sorted(sorted_idx, sorted_idx + 1);
            ++sorted_idx;
        }

        // Vers la gauche
        while (sorted_idx > 0 && sorted_list[sorted_idx] < sorted_list[sorted_idx - 1]) {
            swap_sorted(sorted_idx, sorted_idx - 1);
            --sorted_idx;
        }

        // 5. Lecture directe de la médiane
        return sorted_list[MEDIAN_IDX];
    }

    uint16_t get_median() const {
        return sorted_list[MEDIAN_IDX];
    }

  void print(Stream *stream){
    stream->printf("[%s] size: %d, sorted=[", szName, N);
    for(unsigned int i = 0 ; i < N ; i++){
      stream->printf("%u ", sorted_list[i]);
    }
    stream->printf("], filtered=%u" "\n", get_median());
  }

private:
    inline void swap_sorted(uint8_t i, uint8_t j) {
        // Permutation des valeurs
        uint16_t tmp_val = sorted_list[i];
        sorted_list[i] = sorted_list[j];
        sorted_list[j] = tmp_val;

        // Permutation des pointeurs croisés
        const uint8_t fifo_i = sorted_to_fifo[i];
        const uint8_t fifo_j = sorted_to_fifo[j];

        sorted_to_fifo[i] = fifo_j;
        sorted_to_fifo[j] = fifo_i;

        pos_in_sorted[fifo_i] = j;
        pos_in_sorted[fifo_j] = i;
    }

    uint16_t history[N];
    uint16_t sorted_list[N];
    uint8_t pos_in_sorted[N];
    uint8_t sorted_to_fifo[N];
    const char *szName;

    uint8_t head;
};

#endif // __MEDIAN_FILTER_HPP_INCLUDED__

