#include "../includes/codexion.h"

static int register_a_request(t_coder *coder,
	t_dongle *first,
	t_dongle *second,
	t_request *first_req,
	t_request *second_req)
{
	long long	sequenece;
	int			valid;

	pthread_mutex_lock(&coder->sim->state_mutex);
	sequenece = coder->sim->global_sequence;
	coder->sim->global_sequence++;
	pthread_mutex_unlock(&coder->sim->state_mutex);
	first_req->coder_id = coder->id;
	first_req->sequence = sequenece;
	first_req->deadline = coder->deadline;
	first_req->heap_index = -1;
	second_req->coder_id = coder->id;
	second_req->sequence = sequenece;
	second_req->deadline = coder->deadline;
	second_req->heap_index = -1;
	pthread_mutex_lock(&first->mutex);
	valid = heap_push(&first->queue, first_req);
	pthread_mutex_unlock(&first->mutex);
	if (!valid)
		return (0);
	pthread_mutex_lock(&second->mutex);
	valid = heap_push(&second->queue, second_req);
	pthread_mutex_unlock(&second->mutex);
	if (!valid)
	{
		pthread_mutex_lock(&first->mutex);
		heap_remove(&first->queue, first_req);
		pthread_mutex_unlock(&first->mutex);
		return (0);
	}
	return (1);
}

static int is_my_turn(t_dongle *first,
	t_dongle *second,
	t_request *first_req,
	t_request *second_req
	,long long current_time)
{
	if (first->hold != 0 || second->hold != 0)
		return (0);
	if (current_time < first->available_ms || current_time < second->available_ms)
		return (0);
	if (heap_peek(&first->queue) != first_req
	|| heap_peek(&second->queue) != second_req)
		return (0);
	return (1);
}

static void	claim_dongles(t_dongle *first, t_dongle *second)
{
	heap_pop(&first->queue);
	heap_pop(&second->queue);
	first->hold = 1;
	second->hold = 1;
}

static void cancel_reqeust(t_dongle *first,
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

int	take_dongles_basic(t_coder *coder)
{
    t_dongle	*first;
	t_dongle	*second;
	t_request	*first_req;
	t_request	*second_req;
	long long	current_time;

	if (coder == NULL || coder->sim == NULL)
		return (0);
	if (coder->left == NULL || coder->right == NULL)
		return (0);
	if (coder->left == coder->right)
	{
		log_action(coder->sim, coder->id, "has taken a dongle");
		return (0);
	}
	if (coder->left->id < coder->right->id)
	{
		first = coder->left;
		second = coder->right;
		first_req = &coder->left_request;
		second_req = &coder->right_request;
	}
	else
	{
		first = coder->right;
		second = coder->left;
		first_req = &coder->right_request;
		second_req = &coder->left_request;
	}
	register_a_request(coder, first, second, first_req, second_req);
	while (1)
	{
		pthread_mutex_lock(&coder->sim->state_mutex);
		if (coder->sim->stop_reason != STOP_NONE)
		{
			pthread_mutex_unlock(&coder->sim->state_mutex);
			cancel_reqeust(first, second, first_req, second_req);
			return (0);
		}
		pthread_mutex_unlock(&coder->sim->state_mutex);
		pthread_mutex_lock(&first->mutex);
		pthread_mutex_lock(&second->mutex);
		if (get_time_in_ms(&current_time) == 0)
		{
			pthread_mutex_unlock(&second->mutex);
			pthread_mutex_unlock(&first->mutex);
			cancel_reqeust(first, second, first_req, second_req);
			return (0);
		}
		if (is_my_turn(first, second, first_req, second_req, current_time))
		{
			claim_dongles(first, second);
			pthread_mutex_unlock(&second->mutex);
			pthread_mutex_unlock(&first->mutex);
			log_action(coder->sim, coder->id, "has taken a dongle");
			log_action(coder->sim, coder->id, "has taken a dongle");
			return (1);
		}
		pthread_mutex_unlock(&second->mutex);
		pthread_mutex_unlock(&first->mutex);
		if (sim_sleep(coder->sim, 1) == 0)
		{
			cancel_reqeust(first, second, first_req, second_req);
			return (0);
		}
	}
}