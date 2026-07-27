/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jgaadi <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/24 17:33:09 by jgaadi            #+#    #+#             */
/*   Updated: 2026/07/24 17:33:11 by jgaadi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/codexion.h"

static void	init_coder(t_sim *sim, int index)
{
	t_coder *coder;
	
	coder = &sim->coders[index];
	coder->id = index + 1;
	coder->sim = sim;
	coder->left = &sim->dongles[index];
	coder->right = &sim->dongles[(index + 1) % sim->config.number_of_coders];
	coder->compiles_done = 0;
	coder->finished = (coder->compiles_done >= sim->config.number_of_compiles_required);
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

void cleanup_sim_sync(t_sim *sim)
{
	pthread_mutex_destroy(&sim->arb_mutex);
	pthread_mutex_destroy(&sim->state_mutex);
	pthread_mutex_destroy(&sim->log_mutex);
	pthread_mutex_destroy(&sim->start_mutex);

	pthread_cond_destroy(&sim->arb_cond);
	pthread_cond_destroy(&sim->monitor_cond);
	pthread_cond_destroy(&sim->start_cond);
}

int	sim_init(t_sim *sim, const t_config *config)
{
	int i;

	if (sim == NULL || config == NULL)
		return (0);
	sim->config = *config;
	sim->start_time = 0;
	sim->global_sequence = 0;
	sim->stop_reason = STOP_NONE;
	sim->start_released = 0;
	pthread_mutex_init(&sim->arb_mutex, NULL);
	pthread_mutex_init(&sim->state_mutex, NULL);
	pthread_mutex_init(&sim->log_mutex, NULL);
	pthread_mutex_init(&sim->start_mutex, NULL);

	pthread_cond_init(&sim->arb_cond, NULL);
	pthread_cond_init(&sim->monitor_cond, NULL);
	pthread_cond_init(&sim->start_cond, NULL);
	sim->dongles = malloc(sizeof(t_dongle) * config->number_of_coders);
	if (sim->dongles == NULL)
	{
		cleanup_sim_sync(sim);
		return (0);
	}
	sim->coders = malloc(sizeof(t_coder) * config->number_of_coders);
	if (sim->coders == NULL)
	{
		free(sim->dongles);
		cleanup_sim_sync(sim);
		return (0);
	}
	i = 0;
	while (i < config->number_of_coders)
	{
		if (dongle_init(&sim->dongles[i], i, config->scheduler) == 0)
		{
			while (i > 0)
			{
				i--;
				dongle_destroy(&sim->dongles[i]);
			}
			free(sim->dongles);
			free(sim->coders);
			cleanup_sim_sync(sim);
			return (0);
		}
		i++;
	}
	i = 0;
	while (i < config->number_of_coders)
	{
		init_coder(sim, i);
		i++;
	}
	return (1);
}

void	sim_destroy(t_sim *sim)
{
	int idx;

	if (sim == NULL)
		return ;
	idx = 0;
	while (idx < sim->config.number_of_coders)
	{
		dongle_destroy(&sim->dongles[idx]);
		idx++;
	}
	free(sim->coders);
	free(sim->dongles);
	sim->coders = NULL;
	sim->dongles = NULL;
	cleanup_sim_sync(sim);
	return ;
}

void	wait_for_start(t_sim *sim)
{
	pthread_mutex_lock(&sim->start_mutex);
	while (sim->start_released == 0)
		pthread_cond_wait(&sim->start_cond, &sim->start_mutex);
	pthread_mutex_unlock(&sim->start_mutex);
}

int sim_start_threads(t_sim *sim)
{
	int	idx;
	int	success_created_threads;

	if (sim == NULL)
		return (0);
	success_created_threads = 0;
	while (success_created_threads < sim->config.number_of_coders)
	{
		if (pthread_create(&sim->coders[success_created_threads].thread, NULL,
			coder_routine, &sim->coders[success_created_threads]) != 0)
			break;
		success_created_threads++;
	}
	if (success_created_threads != sim->config.number_of_coders)
	{
		sim->stop_reason = STOP_ERROR;
		pthread_mutex_lock(&sim->start_mutex);
		sim->start_released = 1;
		pthread_cond_broadcast(&sim->start_cond);
		pthread_mutex_unlock(&sim->start_mutex);
		idx = 0;
		while (idx < success_created_threads)
		{
			pthread_join(sim->coders[idx].thread, NULL);
			idx++;
		}
		return (0);
	}
	if (pthread_create(&sim->monitor_thread, NULL,
		monitor_routine, sim) != 0)
		{
			sim->stop_reason = STOP_ERROR;
			pthread_mutex_lock(&sim->start_mutex);
			sim->start_released = 1;
			pthread_cond_broadcast(&sim->start_cond);
			pthread_mutex_unlock(&sim->start_mutex);
			idx = 0;
			while (idx < success_created_threads)
			{
				pthread_join(sim->coders[idx].thread, NULL);
				idx++;
			}
			return (0);
		}
	if (sim_release_start(sim) == 0)
	{
		sim->stop_reason = STOP_ERROR;
		pthread_mutex_lock(&sim->start_mutex);
		sim->start_released = 1;
		pthread_cond_broadcast(&sim->start_cond);
		pthread_mutex_unlock(&sim->start_mutex);
		idx = 0;
		while (idx < success_created_threads)
		{
			pthread_join(sim->coders[idx].thread, NULL);
			idx++;
		}
		pthread_join(sim->monitor_thread, NULL);
		return (0);
	}
	return (1);
}