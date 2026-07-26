/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jgaadi <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/23 11:04:55 by jgaadi            #+#    #+#             */
/*   Updated: 2026/07/23 11:04:56 by jgaadi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "../includes/codexion.h"

int	dongle_init(t_dongle *dongle, int id, t_scheduler scheduler)
{
    if (dongle == NULL)
        return (0);
    dongle->id = id;
    dongle->hold = 0;
    dongle->available_ms = 0;
    if (pthread_mutex_init(&(dongle->mutex), NULL) != 0)
        return (0);
    if (heap_init(&(dongle->queue), 2, scheduler) == 0)
    {
        pthread_mutex_destroy(&(dongle->mutex));     
        return (0);
    }
    return (1);
}

void	dongle_destroy(t_dongle *dongle)
{
    if (dongle == NULL)
        return ;
    heap_destroy(&(dongle->queue));
    pthread_mutex_destroy(&dongle->mutex);
}

int	heap_remove(t_heap *heap, t_request *request)
{
    int idx;
    int parent;
    int moved;

    moved = 0;
    if (heap == NULL || heap->items == NULL)
        return (0);
    if (heap->size == 0)
        return (0);
    if (request == NULL)
        return (0);
    idx = request->heap_index;
    if (idx < 0 || idx >= heap->size || heap->items[idx] != request)
        return (0);
    if (idx < 0 || idx >= heap->size)
        return (0);
    heap->size--;
    if (idx != heap->size)
    {
        moved = 1;
        heap->items[idx] = heap->items[heap->size];
        heap->items[idx]->heap_index = idx;
    }
    request->heap_index = -1;
    heap->items[heap->size] = NULL;
    if (moved)
    {
        parent = (idx - 1) / 2;
        if (idx > 0 && request_with_priority(heap->items[idx],
        heap->items[parent], heap->scheduler))
            shift_up(heap, idx);
        else
            shift_down(heap, idx);
    }
    return (1);
}