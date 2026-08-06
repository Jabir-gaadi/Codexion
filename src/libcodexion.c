/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   libcodexion.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jgaadi <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/06 10:50:55 by jgaadi            #+#    #+#             */
/*   Updated: 2026/08/06 10:50:57 by jgaadi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/codexion.h"

void	join_coders_threads(t_sim *sim, int total_thread)
{
	int	idx;

	idx = 0;
	while (idx < total_thread)
	{
		pthread_join(sim->coders[idx].thread, NULL);
		idx++;
	}
}

int	is_my_turn(t_dongle *first,
	t_dongle *second,
	t_request **arr_req,
	long long current_time)
{
	if (first->hold != 0 || second->hold != 0)
		return (0);
	if (current_time < first->available_ms
		|| current_time < second->available_ms)
		return (0);
	if (heap_peek(&first->queue) != arr_req[0]
		|| heap_peek(&second->queue) != arr_req[1])
		return (0);
	return (1);
}

void	claim_dongles(t_dongle *first, t_dongle *second)
{
	heap_pop(&first->queue);
	heap_pop(&second->queue);
	first->hold = 1;
	second->hold = 1;
}

void	cancel_reqeust(t_dongle *first,
	t_dongle *second,
	t_request *first_req,
	t_request *second_req)
{
	pthread_mutex_lock(&first->mutex);
	if (first_req->heap_index != -1)
		heap_remove(&first->queue, first_req);
	pthread_mutex_unlock(&first->mutex);
	pthread_mutex_lock(&second->mutex);
	if (second_req->heap_index != -1)
		heap_remove(&second->queue, second_req);
	pthread_mutex_unlock(&second->mutex);
}
