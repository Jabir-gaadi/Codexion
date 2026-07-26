#include "./includes/codexion.h"

static int	check_heap(const t_heap *heap)
{
	int	index;
	int	parent;

	if (heap == NULL || heap->items == NULL)
		return (0);
	index = 0;
	while (index < heap->size)
	{
		if (heap->items[index] == NULL)
			return (0);
		if (heap->items[index]->heap_index != index)
			return (0);
		if (index > 0)
		{
			parent = (index - 1) / 2;
			if (request_with_priority(heap->items[index],
					heap->items[parent], heap->scheduler))
				return (0);
		}
		index++;
	}
	return (1);
}

static int	check_pop_order(t_heap *heap, int *expected, int count)
{
	t_request	*request;
	int			index;

	index = 0;
	while (index < count)
	{
		request = heap_pop(heap);
		if (request == NULL || request->coder_id != expected[index])
			return (0);
		if (request->heap_index != -1)
			return (0);
		if (heap->size > 0 && !check_heap(heap))
			return (0);
		index++;
	}
	return (heap_pop(heap) == NULL);
}

static int	test_invalid_init(void)
{
	t_heap	heap;

	if (heap_init(NULL, 4, CODEX_FIFO))
		return (0);
	if (heap_init(&heap, 0, CODEX_FIFO))
		return (0);
	if (heap_init(&heap, 4, (t_scheduler)99))
		return (0);
	return (1);
}

static int	test_fifo(void)
{
	t_heap		heap;
	t_request	a;
	t_request	b;
	t_request	c;
	t_request	d;
	t_request	extra;
	int			expected[4];

	a = (t_request){3, 2, 10, -1};
	b = (t_request){2, 1, 999, -1};
	c = (t_request){1, 1, 500, -1};
	d = (t_request){4, 3, 1, -1};
	extra = (t_request){5, 0, 0, -1};
	expected[0] = 1;
	expected[1] = 2;
	expected[2] = 3;
	expected[3] = 4;
	if (!heap_init(&heap, 4, CODEX_FIFO))
		return (0);
	if (!heap_push(&heap, &a) || !check_heap(&heap)
		|| !heap_push(&heap, &b) || !check_heap(&heap)
		|| !heap_push(&heap, &c) || !check_heap(&heap)
		|| !heap_push(&heap, &d) || !check_heap(&heap))
		return (heap_destroy(&heap), 0);
	if (heap_push(&heap, &extra) || heap.size != 4 || extra.heap_index != -1)
		return (heap_destroy(&heap), 0);
	if (!check_pop_order(&heap, expected, 4))
		return (heap_destroy(&heap), 0);
	heap_destroy(&heap);
	return (heap.items == NULL && heap.size == 0 && heap.capacity == 0);
}

static int	test_edf(void)
{
	t_heap		heap;
	t_request	a;
	t_request	b;
	t_request	c;
	t_request	d;
	int			expected[4];

	a = (t_request){1, 4, 200, -1};
	b = (t_request){2, 3, 100, -1};
	c = (t_request){3, 1, 100, -1};
	d = (t_request){1, 1, 100, -1};
	expected[0] = 1;
	expected[1] = 3;
	expected[2] = 2;
	expected[3] = 1;
	if (!heap_init(&heap, 4, CODEX_EDF))
		return (0);
	if (!heap_push(&heap, &a) || !heap_push(&heap, &b)
		|| !heap_push(&heap, &c) || !heap_push(&heap, &d))
		return (heap_destroy(&heap), 0);
	if (!check_heap(&heap) || !check_pop_order(&heap, expected, 4))
		return (heap_destroy(&heap), 0);
	heap_destroy(&heap);
	return (1);
}

static int	test_destroy_nonempty(void)
{
	t_heap		heap;
	t_request	a;
	t_request	b;

	a = (t_request){1, 1, 10, -1};
	b = (t_request){2, 2, 20, -1};
	if (!heap_init(&heap, 2, CODEX_FIFO))
		return (0);
	if (!heap_push(&heap, &a) || !heap_push(&heap, &b))
		return (heap_destroy(&heap), 0);
	heap_destroy(&heap);
	return (a.heap_index == -1 && b.heap_index == -1
		&& heap.items == NULL && heap.size == 0 && heap.capacity == 0);
}

int	main(void)
{
	int	failed;

	failed = 0;
	if (!test_invalid_init())
		printf("FAIL: invalid initialization\n"), failed++;
	if (!test_fifo())
		printf("FAIL: FIFO ordering or invariants\n"), failed++;
	if (!test_edf())
		printf("FAIL: EDF ordering or invariants\n"), failed++;
	if (!test_destroy_nonempty())
		printf("FAIL: destroy nonempty heap\n"), failed++;
	if (failed == 0)
		printf("PASS: all implemented heap tests\n");
	return (failed != 0);
}
