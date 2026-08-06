/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jgaadi <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/04 18:38:39 by jgaadi            #+#    #+#             */
/*   Updated: 2026/08/04 18:38:40 by jgaadi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/codexion.h"

static int	update_compile_start(t_coder *coder)
{
	long long	current_time;
	long long	deadline;

	if (get_time_in_ms(&current_time) == 0)
		return (0);
	if (add_time_ms(current_time,
			coder->sim->config.time_to_burnout, &deadline) == 0)
		return (0);
	pthread_mutex_lock(&coder->sim->state_mutex);
	if (coder->sim->stop_reason != STOP_NONE)
	{
		pthread_mutex_unlock(&coder->sim->state_mutex);
		return (0);
	}
	coder->start_of_lastcompile = current_time;
	coder->deadline = deadline;
	pthread_mutex_unlock(&coder->sim->state_mutex);
	return (1);
}

static int	compile_phase(t_coder *coder)
{
	int	compile_completed;

	if (take_dongles_basic(coder) == 0)
		return (0);
	if (update_compile_start(coder) == 0)
	{
		release_dongles_basic(coder);
		return (0);
	}
	log_action(coder->sim, coder->id, "is compiling");
	compile_completed = sim_sleep(coder->sim,
			coder->sim->config.time_to_compile);
	if (release_dongles_basic(coder) == 0)
		return (0);
	return (compile_completed);
}

static int	debug_refactor_phase(t_coder *coder)
{
	log_action(coder->sim, coder->id, "is debugging");
	if (sim_sleep(coder->sim,
			coder->sim->config.time_to_debug) == 0)
		return (0);
	log_action(coder->sim, coder->id, "is refactoring");
	if (sim_sleep(coder->sim,
			coder->sim->config.time_to_refactor) == 0)
		return (0);
	return (1);
}

static int	register_compile(t_coder *coder)
{
	int	finished;

	pthread_mutex_lock(&coder->sim->state_mutex);
	coder->compiles_done++;
	if (coder->compiles_done
		== coder->sim->config.number_of_compiles_required)
		coder->finished = 1;
	finished = coder->finished;
	pthread_cond_signal(&coder->sim->monitor_cond);
	pthread_mutex_unlock(&coder->sim->state_mutex);
	return (finished);
}

void	*coder_routine(void *args)
{
	t_coder	*coder;

	if (args == NULL)
		return (NULL);
	coder = (t_coder *)args;
	if (coder->sim == NULL)
		return (NULL);
	wait_for_start(coder->sim);
	if (coder->id % 2 == 0)
		usleep(500);
	while (sim_is_running(coder->sim))
	{
		if (compile_phase(coder) == 0)
			return (NULL);
		if (debug_refactor_phase(coder) == 0)
			return (NULL);
		if (register_compile(coder))
			return (NULL);
	}
	return (NULL);
}
