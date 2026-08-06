/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jgaadi <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/19 00:00:00 by jgaadi            #+#    #+#             */
/*   Updated: 2026/07/19 00:00:00 by jgaadi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/codexion.h"

static int	fifo_compare(const t_request *x, const t_request *y)
{
	if (x->sequence < y->sequence)
		return (1);
	if (x->sequence > y->sequence)
		return (0);
	if (x->coder_id < y->coder_id)
		return (1);
	return (0);
}

static int	edf_compare(const t_request *x, const t_request *y)
{
	if (x->deadline < y->deadline)
		return (1);
	if (x->deadline > y->deadline)
		return (0);
	return (fifo_compare(x, y));
}

int	request_with_priority(
	const	t_request *x,
	const t_request *y,
	t_scheduler scheduler)
{
	if (scheduler == CODEX_FIFO)
		return (fifo_compare(x, y));
	else if (scheduler == CODEX_EDF)
		return (edf_compare(x, y));
	return (0);
}

int	heap_init(t_heap *heap, int capacity, t_scheduler scheduler)
{
	if (capacity != 2)
		return (0);
	heap->items[0] = NULL;
	heap->items[1] = NULL;
	heap->size = 0;
	heap->scheduler = scheduler;
	return (1);
}
