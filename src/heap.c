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

void	shift_up(t_heap *heap, int index)
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

	if ((scheduler != CODEX_EDF) && (scheduler != CODEX_FIFO))
		return (0);
	if (heap == NULL)
		return (0);
	if (capacity <= 0)
		return (0);
	if ((unsigned long)capacity > SIZE_MAX / sizeof(*request_arr))
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

int	heap_push(t_heap *heap, t_request *request)
{
	int	idx;

	if ((heap == NULL) || (request == NULL))
		return (0);
	if (heap->items == NULL)
		return (0);
	if ((heap->size < 0) || (heap->size >= heap->capacity))
		return (0);
	idx = heap->size;
	heap->items[idx] = request;
	request->heap_index = idx;
	heap->size++;
	shift_up(heap, idx);
	return (1);
}

void	shift_down(t_heap *heap, int index)
{
	int	left_idx;
	int	right_idx;
	int selected;

	while (1)
	{
		left_idx = index * 2 + 1;
		right_idx = index * 2 + 2;
		selected = index;
		if ((left_idx < heap->size) && (request_with_priority(heap->items[left_idx], heap->items[selected], heap->scheduler)))
			selected = left_idx;
		if ((right_idx < heap->size) && (request_with_priority(heap->items[right_idx], heap->items[selected], heap->scheduler)))
			selected = right_idx;
		if (selected == index)
			break;
		swap_requests(heap, index, selected);
		index = selected;
	}
}

t_request	*heap_pop(t_heap *heap)
{
	t_request *tmp_request;

	if (heap == NULL)
		return (NULL);
	if (heap->items == NULL)
		return (NULL);
	if (heap->size == 0)
		return (NULL);
	tmp_request = heap->items[0];
	heap->size--;
	if (heap->size > 0)
	{
		heap->items[0] = heap->items[heap->size];
		heap->items[0]->heap_index = 0;
		shift_down(heap, 0);
	}
	heap->items[heap->size] = NULL;
	tmp_request->heap_index = -1;
	return (tmp_request);
}

void	heap_destroy(t_heap *heap)
{
	int	i;

	if (heap == NULL)
		return ;
	i = 0;
	while(i < heap->size)
	{
		heap->items[i]->heap_index = -1;
		i++;
	}
	free(heap->items);
	heap->items = NULL;
	heap->size = 0;
	heap->capacity = 0;
}

t_request	*heap_peek(t_heap *heap)
{
	if (heap == NULL)
		return (NULL);
	if (heap->items == NULL)
		return (NULL);
	if (heap->size == 0)
		return (NULL);
	return (heap->items[0]);
}

int	heap_remove(t_heap *heap, t_request *request)
{
	int	index;

	if (heap == NULL || request == NULL)
		return (0);
	if (heap->items == NULL || heap->size == 0)
		return (0);
	index = request->heap_index;
	if (index < 0 || index >= heap->size)
		return (0);
	if (heap->items[index] != request)
		return (0);
	heap->size--;
	if (index != heap->size)
	{
		heap->items[index] = heap->items[heap->size];
		heap->items[index]->heap_index = index;
		if ((index > 0) && request_with_priority(heap->items[index],
			heap->items[(index - 1) / 2], heap->scheduler))
			shift_up(heap, index);
		else
			shift_down(heap, index);
		}
	heap->items[heap->size] = NULL;
	request->heap_index = -1;
	return (1);
}