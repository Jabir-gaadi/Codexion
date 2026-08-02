/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init2.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jgaadi <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/02 10:26:33 by jgaadi            #+#    #+#             */
/*   Updated: 2026/08/02 10:26:34 by jgaadi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/codexion.h"

void	wait_for_start(t_sim *sim)
{
	pthread_mutex_lock(&sim->start_mutex);
	while (sim->start_released == 0)
		pthread_cond_wait(&sim->start_cond, &sim->start_mutex);
	pthread_mutex_unlock(&sim->start_mutex);
}

void	init_coder(t_sim *sim, int index)
{
	t_coder	*coder;

	coder = &sim->coders[index];
	coder->id = index + 1;
	coder->sim = sim;
	coder->left = &sim->dongles[index];
	coder->right = &sim->dongles[(index + 1) % sim->config.number_of_coders];
	coder->compiles_done = 0;
	coder->finished = (
			coder->compiles_done >= sim->config.number_of_compiles_required
			);
	coder->start_of_lastcompile = 0;
	coder->deadline = 0;
	coder->left_request.coder_id = coder->id;
	coder->left_request.deadline = 0;
	coder->left_request.sequence = 0;
	coder->left_request.heap_index = -1;
	coder->right_request.coder_id = coder->id;
	coder->right_request.deadline = 0;
	coder->right_request.sequence = 0;
	coder->right_request.heap_index = -1;
}

static int	creation_threads(t_sim *sim)
{
	int	success_created;

	success_created = 0;
	while (success_created < sim->config.number_of_coders)
	{
		if (pthread_create(&sim->coders[success_created].thread, NULL,
				coder_routine, &sim->coders[success_created]) != 0)
			break ;
		success_created++;
	}
	if (success_created > sim->config.number_of_coders)
	{
		sim->stop_reason = STOP_ERROR;
		pthread_mutex_lock(&sim->state_mutex);
		sim->start_released = 1;
		pthread_cond_broadcast(&sim->start_cond);
		pthread_mutex_unlock(&sim->start_mutex);
		join_coders_threads(sim, success_created);
		return (0);
	}
	return (success_created);
}

static int	creation_monitor_thread(t_sim *sim, int total_threads)
{
	if (pthread_create(&sim->monitor_thread, NULL, monitor_routine, sim) != 0)
	{
		sim->stop_reason = STOP_ERROR;
		pthread_mutex_lock(&sim->start_mutex);
		sim->start_released = 1;
		pthread_cond_broadcast(&sim->monitor_cond);
		pthread_mutex_unlock(&sim->start_mutex);
		join_coders_threads(sim, total_threads);
		return (0);
	}
	return (1);
}

int	sim_start_threads(t_sim *sim)
{
	int	success_created_threads;

	if (sim == NULL)
		return (0);
	success_created_threads = creation_threads(sim);
	if (success_created_threads == 0)
		return (0);
	if (creation_monitor_thread(sim, success_created_threads) == 0)
		return (0);
	if (sim_release_start(sim) == 0)
	{
		sim->stop_reason = STOP_ERROR;
		pthread_mutex_lock(&sim->start_mutex);
		sim->start_released = 1;
		pthread_cond_broadcast(&sim->start_cond);
		pthread_mutex_unlock(&sim->start_mutex);
		join_coders_threads(sim, success_created_threads);
		pthread_join(sim->monitor_thread, NULL);
		return (0);
	}
	return (1);
}
