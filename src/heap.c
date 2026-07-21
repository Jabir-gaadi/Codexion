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
	if (x->sequence > y->sequence)
		return (0);
	if (x->sequence < y->sequence)
		return (1);
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
	if ((x == NULL) || (y == NULL))
		return (0);
	if (scheduler == CODEX_FIFO)
		return (fifo_compare(x, y));
	else if (scheduler == CODEX_EDF)
		return (edf_compare(x, y));
	return (0);
}

static void	swap_requests(t_heap *heap, int first, int second)
{
	t_request	*tmp;

	if (heap == NULL)
		return ;
	tmp = heap->items[first];
	heap->items[first] = heap->items[second];
	heap->items[second] = tmp;
	heap->items[first]->heap_index = first;
	heap->items[second]->heap_index = second;
}

static void	shift_up(t_heap *heap, int index)
{
	int	parent;

	while (index > 0)
	{
		parent = (index - 1) / 2;
		if (
			!(request_with_priority(heap->items[index],
					heap->items[parent], heap->scheduler))
		)
			break ;
		swap_requests(heap, index, parent);
		index = parent;
	}
}

int	heap_init(t_heap *heap, int capacity, t_scheduler scheduler)
{
	t_request	**request_arr;

	if (heap == NULL)
		return (0);
	if (capacity <= 0)
		return (0);
	if (capacity > SIZE_MAX / sizeof(*request_arr))
		return (0);
	request_arr = malloc((capacity * sizeof(*request_arr)));
	if (request_arr == NULL)
		return (0);
	heap->items = request_arr;
	heap->size = 0;
	heap->capacity = capacity;
	heap->scheduler = scheduler;
	return (1);
}
