/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   time_utils.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jgaadi <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/19 12:08:10 by jgaadi            #+#    #+#             */
/*   Updated: 2026/07/19 12:08:12 by jgaadi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/codexion.h"

int	get_time_in_ms(long long *result)
{
	struct timeval		time_now;
	long long			time_sec;
	long long			sec_to_ms;
	long long			micro_to_ms;
	long long			ms_time;

	if (gettimeofday(&time_now, NULL) == -1)
		return (0);
	time_sec = time_now.tv_sec;
	if (time_sec < 0)
		return (0);
	if (time_sec > LLONG_MAX / 1000)
		return (0);
	sec_to_ms = time_sec * 1000;
	micro_to_ms = time_now.tv_usec / 1000;
	if (micro_to_ms > LLONG_MAX - sec_to_ms)
		return (0);
	ms_time = sec_to_ms + micro_to_ms;
	*result = ms_time;
	return (1);
}

int	add_time_ms(long long base, long long duration, long long *result)
{
	if (duration > LLONG_MAX - base)
		return (0);
	*result = duration + base;
	return (1);
}

int	ms_to_timespec(long long ms_time, struct timespec *result)
{
	if (result == NULL)
		return (0);
	if (ms_time < 0)
		return (0);
	result->tv_sec = ms_time / 1000;
	result->tv_nsec = (ms_time % 1000) * 1000000;
	return (1);
}
