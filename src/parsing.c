/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parsing.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jgaadi <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/17 15:52:17 by jgaadi            #+#    #+#             */
/*   Updated: 2026/07/17 15:52:19 by jgaadi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/codexion.h"

static int	parse_numbers(const char *str, long long max, long long *result)
{
	long long	value;

	int (i), (digit);
	if (str == NULL || result == NULL)
		return (0);
	if (max < 0)
		return (0);
	if (*str == '\0')
		return (0);
	value = 0;
	i = 0;
	while (str[i])
	{
		if (str[i] < '0' || str[i] > '9')
			return (0);
		digit = str[i] - '0';
		if (value > max / 10)
			return (4);
		if ((value == max / 10) && (digit > max % 10))
			return (4);
		value = value * 10 + digit;
		i++;
	}
	*result = value;
	return (1);
}

static int	parse_scheduler(const char *str, t_scheduler *scheduler)
{
	if ((str == NULL) || (scheduler == NULL))
		return (0);
	if (strcmp(str, "fifo") == 0)
	{
		*scheduler = CODEX_FIFO;
		return (1);
	}
	else if (strcmp(str, "edf") == 0)
	{
		*scheduler = CODEX_EDF;
		return (1);
	}
	return (0);
}

static int	parse_numeric_args(const char *str, int index, long long *value)
{
	long long	max;
	int			check;

	max = LLONG_MAX;
	if (index == 1 || index == 6)
		max = INT_MAX;
	check = parse_numbers(str, max, value);
	if (check == 4)
	{
		printf("ARGument number %d can overflow!\n", index);
		return (0);
	}
	if (check == 0)
		return (0);
	if ((index == 1) && (*value < 1))
		return (0);
	return (1);
}

static void	fill_config(int index, long long value, t_config	*config)
{
	if (index == 1)
		config->number_of_coders = (int)value;
	else if (index == 2)
		config->time_to_burnout = value;
	else if (index == 3)
		config->time_to_compile = value;
	else if (index == 4)
		config->time_to_debug = value;
	else if (index == 5)
		config->time_to_refactor = value;
	else if (index == 6)
		config->number_of_compiles_required = (int)value;
	else if (index == 7)
		config->dongle_cooldown = value;
}

int	parse_args(int ac, char *av[], t_config *config)
{
	t_config	tmp;
	long long	value;
	int			i;

	if (av == NULL || config == NULL || ac != 9)
		return (0);
	i = 1;
	while (i < 8)
	{
		if ((parse_numeric_args(av[i], i, &value)) == 0)
			return (2);
		fill_config(i, value, &tmp);
		i++;
	}
	if ((parse_scheduler(av[8], &(tmp.scheduler))) != 1)
		return (3);
	*config = tmp;
	return (1);
}
