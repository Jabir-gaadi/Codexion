/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   monitor.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jgaadi <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/06 09:08:12 by jgaadi            #+#    #+#             */
/*   Updated: 2026/08/06 09:08:16 by jgaadi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/codexion.h"

static int	check_coders(t_sim *sim, long long current_time,
	int idx, int all_compile)
{
	while (idx < sim->config.number_of_coders)
	{
		if (sim->coders[idx].compiles_done
			< sim->config.number_of_compiles_required)
			all_compile = 0;
		if (sim->coders[idx].compiles_done
			< sim->config.number_of_compiles_required
			&& current_time >= sim->coders[idx].deadline)
		{
			pthread_mutex_unlock(&sim->state_mutex);
			sim_request_stop(sim, STOP_BURNOUT, sim->coders[idx].id,
				current_time - sim->start_time);
			return (0);
		}
		idx++;
	}
	if (all_compile)
	{
		pthread_mutex_unlock(&sim->state_mutex);
		sim_request_stop(sim, STOP_COMPLETED, 0,
			current_time - sim->start_time);
		return (0);
	}
	return (1);
}

void	*monitor_routine(void *args)
{
	t_sim		*sim;
	long long	current_time;

	if (args == NULL)
		return (NULL);
	sim = (t_sim *)args;
	wait_for_start(sim);
	while (1)
	{
		if (get_time_in_ms(&current_time) == 0)
			return (sim_request_stop(sim, STOP_ERROR, 0, 0), NULL);
		pthread_mutex_lock(&sim->state_mutex);
		if (sim->stop_reason != STOP_NONE)
		{
			pthread_mutex_unlock(&sim->state_mutex);
			return (NULL);
		}
		if (check_coders(sim, current_time, 0, 1) == 0)
			return (NULL);
		pthread_mutex_unlock(&sim->state_mutex);
		usleep(1000);
	}
	return (NULL);
}
