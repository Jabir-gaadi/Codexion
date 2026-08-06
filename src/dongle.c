/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jgaadi <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/23 11:04:55 by jgaadi            #+#    #+#             */
/*   Updated: 2026/07/23 11:04:56 by jgaadi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/codexion.h"

int	dongle_init(t_dongle *dongle, int id, t_scheduler scheduler)
{
	if (dongle == NULL)
		return (0);
	dongle->id = id;
	dongle->hold = 0;
	dongle->available_ms = 0;
	if (pthread_mutex_init(&(dongle->mutex), NULL) != 0)
		return (0);
	if (heap_init(&(dongle->queue), 2, scheduler) == 0)
	{
		pthread_mutex_destroy(&(dongle->mutex));
		return (0);
	}
	return (1);
}

void	dongle_destroy(t_dongle *dongle)
{
	if (dongle == NULL)
		return ;
	heap_destroy(&(dongle->queue));
	pthread_mutex_destroy(&dongle->mutex);
}

static int	choose_to_lock(t_dongle **first, t_dongle **second, t_coder *coder)
{
	if (coder->left == coder->right)
		return (0);
	if (coder->left->id < coder->right->id)
	{
		*first = coder->left;
		*second = coder->right;
	}
	else
	{
		*first = coder->right;
		*second = coder->left;
	}
	return (1);
}

int	release_dongles_basic(t_coder *coder)
{
	t_dongle	*first;
	t_dongle	*second;
	long long	current_time;
	long long	available_time;

	if (choose_to_lock(&first, &second, coder) == 0)
		return (0);
	pthread_mutex_lock(&first->mutex);
	pthread_mutex_lock(&second->mutex);
	if ((get_time_in_ms(&current_time) == 0)
		|| (add_time_ms(current_time,
				coder->sim->config.dongle_cooldown,
				&available_time) == 0))
	{
		pthread_mutex_unlock(&second->mutex);
		pthread_mutex_unlock(&first->mutex);
		return (0);
	}
	coder->left->hold = 0;
	coder->right->hold = 0;
	coder->left->available_ms = available_time;
	coder->right->available_ms = available_time;
	pthread_mutex_unlock(&second->mutex);
	pthread_mutex_unlock(&first->mutex);
	return (1);
}
