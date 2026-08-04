/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   simulation.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jgaadi <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/02 11:50:56 by jgaadi            #+#    #+#             */
/*   Updated: 2026/08/02 11:50:58 by jgaadi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/codexion.h"

int	sim_release_start(t_sim *sim)
{
	long long	current_time;
	long long	initial_deadline;
	int			idx;

	if (!get_time_in_ms(&current_time))
		return (0);
	if (!add_time_ms(current_time, sim->config.time_to_burnout,
			&initial_deadline))
		return (0);
	pthread_mutex_lock(&sim->start_mutex);
	sim->start_time = current_time;
	idx = 0;
	while (idx < sim->config.number_of_coders)
	{
		sim->coders[idx].start_of_lastcompile = sim->start_time;
		sim->coders[idx].deadline = initial_deadline;
		idx++;
	}
	sim->start_released = 1;
	pthread_cond_broadcast(&sim->start_cond);
	pthread_mutex_unlock(&sim->start_mutex);
	return (1);
}

int	sim_join_threads(t_sim *sim)
{
	int	success_join;
	int	idx;

	if (sim == NULL)
		return (0);
	success_join = 1;
	idx = 0;
	while (idx < sim->config.number_of_coders)
	{
		if (pthread_join(sim->coders[idx].thread, NULL) != 0)
			success_join = 0;
		idx++;
	}
	if (pthread_join(sim->monitor_thread, NULL) != 0)
		success_join = 0;
	return (success_join);
}

int	sim_sleep(t_sim	*sim, long long duration)
{
	long long		current_time;
	long long		end_time;
	int				completed;
	struct timespec	ts;

	completed = 0;
	if (get_time_in_ms(&current_time) == 0)
		sim_request_stop(sim, STOP_ERROR, 0, 0);
	add_time_ms(current_time, duration, &end_time);
	ms_to_timespec(end_time, &ts);
	pthread_mutex_lock(&sim->state_mutex);
	while (sim->stop_reason == STOP_NONE)
	{
		get_time_in_ms(&current_time);
		if (current_time >= end_time)
			break ;
		pthread_cond_timedwait(&sim->monitor_cond, &sim->state_mutex, &ts);
	}
	if (sim->stop_reason == STOP_NONE && current_time >= end_time)
		completed = 1;
	pthread_mutex_unlock(&sim->state_mutex);
	return (completed);
}

int	sim_request_stop(t_sim *sim, t_stop_reason reason,
		int coder_id, long long terminal_time)
{
	if (sim == NULL || reason == STOP_NONE)
		return (0);
	pthread_mutex_lock(&sim->log_mutex);
	pthread_mutex_lock(&sim->state_mutex);
	if (sim->stop_reason != STOP_NONE)
	{
		pthread_mutex_unlock(&sim->state_mutex);
		pthread_mutex_unlock(&sim->log_mutex);
		return (0);
	}
	sim->stop_reason = reason;
	pthread_cond_broadcast(&sim->monitor_cond);
	pthread_mutex_unlock(&sim->state_mutex);
	if (reason == STOP_BURNOUT)
		printf("%lld %d burned out\n", terminal_time, coder_id);
	pthread_mutex_unlock(&sim->log_mutex);
	pthread_mutex_lock(&sim->arb_mutex);
	pthread_cond_broadcast(&sim->arb_cond);
	pthread_mutex_unlock(&sim->arb_mutex);
	pthread_mutex_lock(&sim->start_mutex);
	sim->start_released = 1;
	pthread_cond_broadcast(&sim->start_cond);
	pthread_mutex_unlock(&sim->start_mutex);
	return (1);
}

int	sim_is_running(t_sim *sim)
{
	pthread_mutex_lock(&sim->state_mutex);
	if (sim->stop_reason != STOP_NONE)
	{
		pthread_mutex_unlock(&sim->state_mutex);
		return (0);
	}
	pthread_mutex_unlock(&sim->state_mutex);
	return (1);
}
