/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap2.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jgaadi <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/04 18:07:40 by jgaadi            #+#    #+#             */
/*   Updated: 2026/08/04 18:07:41 by jgaadi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/codexion.h"

t_request	*heap_peek(t_heap *heap)
{
	if (heap == NULL)
		return (NULL);
	if (heap->size == 0)
		return (NULL);
	return (heap->items[0]);
}

int	heap_remove(t_heap *heap, t_request *request)
{
	int	index;

	if (heap->size == 0)
		return (0);
	index = request->heap_index;
	if (index < 0 || index >= heap->size)
		return (0);
	if (heap->items[index] != request)
		return (0);
	if (index == 0 && heap->size == 2)
	{
		heap->items[0] = heap->items[1];
		heap->items[0]->heap_index = 0;
	}
	else if (index == 0)
		heap->items[0] = NULL;
	heap->items[1] = NULL;
	heap->size--;
	request->heap_index = -1;
	return (1);
}

int	heap_push(t_heap *heap, t_request *request)
{
	if (heap->size == 0)
	{
		heap->items[0] = request;
		request->heap_index = 0;
		heap->size = 1;
		return (1);
	}
	if (request_with_priority(request, heap->items[0], heap->scheduler))
	{
		heap->items[1] = heap->items[0];
		heap->items[1]->heap_index = 1;
		heap->items[0] = request;
		request->heap_index = 0;
	}
	else
	{
		heap->items[1] = request;
		request->heap_index = 1;
	}
	heap->size = 2;
	return (1);
}

t_request	*heap_pop(t_heap *heap)
{
	t_request	*tmp_request;

	tmp_request = heap->items[0];
	if (heap->size == 2)
	{
		heap->items[0] = heap->items[1];
		heap->items[0]->heap_index = 0;
	}
	else
		heap->items[0] = NULL;
	heap->items[1] = NULL;
	heap->size--;
	tmp_request->heap_index = -1;
	return (tmp_request);
}

void	heap_destroy(t_heap *heap)
{
	if (heap == NULL)
		return ;
	if (heap->size > 0)
		heap->items[0]->heap_index = -1;
	if (heap->size > 1)
		heap->items[1]->heap_index = -1;
	heap->items[0] = NULL;
	heap->items[1] = NULL;
	heap->size = 0;
}
