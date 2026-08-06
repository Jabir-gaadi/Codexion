/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   acquire.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jgaadi <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/06 09:52:23 by jgaadi            #+#    #+#             */
/*   Updated: 2026/08/06 09:52:25 by jgaadi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/codexion.h"

static void	fill_value_request(t_coder *coder, t_request **arr_req,
long long sequenece)
{
	arr_req[0]->coder_id = coder->id;
	arr_req[0]->sequence = sequenece;
	arr_req[0]->deadline = coder->deadline;
	arr_req[0]->heap_index = -1;
	arr_req[1]->coder_id = coder->id;
	arr_req[1]->sequence = sequenece;
	arr_req[1]->deadline = coder->deadline;
	arr_req[1]->heap_index = -1;
}

static int	register_a_request(t_coder *coder,
	t_dongle *first,
	t_dongle *second,
	t_request **arr_req)
{
	long long	sequenece;
	int			valid;

	pthread_mutex_lock(&coder->sim->state_mutex);
	sequenece = coder->sim->global_sequence;
	coder->sim->global_sequence++;
	pthread_mutex_unlock(&coder->sim->state_mutex);
	fill_value_request(coder, arr_req, sequenece);
	pthread_mutex_lock(&first->mutex);
	valid = heap_push(&first->queue, arr_req[0]);
	pthread_mutex_unlock(&first->mutex);
	if (!valid)
		return (0);
	pthread_mutex_lock(&second->mutex);
	valid = heap_push(&second->queue, arr_req[1]);
	pthread_mutex_unlock(&second->mutex);
	if (!valid)
	{
		pthread_mutex_lock(&first->mutex);
		heap_remove(&first->queue, arr_req[0]);
		pthread_mutex_unlock(&first->mutex);
		return (0);
	}
	return (1);
}

static int	take_both_dngls(t_coder *coder, t_dongle *first, t_dongle *second)
{
	claim_dongles(first, second);
	pthread_mutex_unlock(&second->mutex);
	pthread_mutex_unlock(&first->mutex);
	log_action(coder->sim, coder->id, "has taken a dongle");
	return (log_action(coder->sim, coder->id, "has taken a dongle"), 1);
}

static int	loop_take_dngls(t_coder *coder, t_dongle *first, t_dongle *second,
t_request **arr_req)
{
	long long	current_time;

	while (1)
	{
		pthread_mutex_lock(&coder->sim->state_mutex);
		if (coder->sim->stop_reason != STOP_NONE)
		{
			pthread_mutex_unlock(&coder->sim->state_mutex);
			return (cancel_reqeust(first, second, arr_req[0], arr_req[1]), 0);
		}
		pthread_mutex_unlock(&coder->sim->state_mutex);
		pthread_mutex_lock(&first->mutex);
		pthread_mutex_lock(&second->mutex);
		get_time_in_ms(&current_time);
		if (is_my_turn(first, second, arr_req, current_time))
			return (take_both_dngls(coder, first, second));
		pthread_mutex_unlock(&second->mutex);
		pthread_mutex_unlock(&first->mutex);
		if (sim_sleep(coder->sim, 1) == 0)
		{
			return (cancel_reqeust(first, second, arr_req[0], arr_req[1]), 0);
		}
	}
}

int	take_dongles_basic(t_coder *coder)
{
	t_request	*arr_req[2];

	t_dongle (*first),
	(*second);
	t_request (*first_req),
	(*second_req);
	if (coder->left == coder->right)
		return (log_action(coder->sim, coder->id, "has taken a dongle"), 0);
	first = coder->right;
	second = coder->left;
	first_req = &coder->right_request;
	second_req = &coder->left_request;
	if (coder->left->id < coder->right->id)
	{
		first = coder->left;
		second = coder->right;
		first_req = &coder->left_request;
		second_req = &coder->right_request;
	}
	arr_req[0] = first_req;
	arr_req[1] = second_req;
	register_a_request(coder, first, second, arr_req);
	return (loop_take_dngls(coder, first, second, arr_req));
}
