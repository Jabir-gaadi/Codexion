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

void	init_sim_or_destroy(t_sim *sim, int select)
{
	if (select == 0)
	{
		pthread_mutex_destroy(&sim->arb_mutex);
		pthread_mutex_destroy(&sim->state_mutex);
		pthread_mutex_destroy(&sim->log_mutex);
		pthread_mutex_destroy(&sim->start_mutex);
		pthread_cond_destroy(&sim->arb_cond);
		pthread_cond_destroy(&sim->monitor_cond);
		pthread_cond_destroy(&sim->start_cond);
	}
	else
	{
		pthread_mutex_init(&sim->arb_mutex, NULL);
		pthread_mutex_init(&sim->state_mutex, NULL);
		pthread_mutex_init(&sim->log_mutex, NULL);
		pthread_mutex_init(&sim->start_mutex, NULL);
		pthread_cond_init(&sim->arb_cond, NULL);
		pthread_cond_init(&sim->monitor_cond, NULL);
		pthread_cond_init(&sim->start_cond, NULL);
	}
}

static int	allocate_sim_array(t_sim *sim)
{
	int	count;

	count = sim->config.number_of_coders;
	sim->dongles = malloc(sizeof(t_dongle) * count);
	if (sim->dongles == NULL)
		return (init_sim_or_destroy(sim, 0), 0);
	sim->coders = malloc(sizeof(t_coder) * count);
	if (sim->coders == NULL)
	{
		free(sim->dongles);
		init_sim_or_destroy(sim, 0);
		return (0);
	}
	return (1);
}

static int	dongles_array_init(t_sim *sim)
{
	int	i;

	i = 0;
	while (i < sim->config.number_of_coders)
	{
		if (dongle_init(&sim->dongles[i], i, sim->config.scheduler) == 0)
		{
			i--;
			while (i >= 0)
			{
				dongle_destroy(&sim->dongles[i]);
				i--;
			}
			free(sim->dongles);
			free(sim->coders);
			init_sim_or_destroy(sim, 0);
			return (0);
		}
		i++;
	}
	return (1);
}

int	sim_init(t_sim *sim, const t_config *config)
{
	int	i;

	sim->config = *config;
	sim->start_time = 0;
	sim->global_sequence = 0;
	sim->stop_reason = STOP_NONE;
	sim->start_released = 0;
	init_sim_or_destroy(sim, 1);
	if (allocate_sim_array(sim) == 0)
		return (0);
	if (dongles_array_init(sim) == 0)
		return (0);
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
	int	idx;

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
	init_sim_or_destroy(sim, 0);
	return ;
}
