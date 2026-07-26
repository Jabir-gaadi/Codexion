/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jgaadi <marvin@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/17 15:26:24 by jgaadi            #+#    #+#             */
/*   Updated: 2026/07/17 15:26:25 by jgaadi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CODEXION_H
# define CODEXION_H

# include <stdio.h>
# include <string.h>
# include <limits.h>
# include <stdlib.h>
# include <sys/time.h>
# include <time.h>
<<<<<<< HEAD
#include <stdint.h>


=======
# include <stdint.h>
# include <pthread.h>

// t_scheduler
// t_stop_reason
// t_config
// t_request
// t_heap
// t_dongle
// forward declaration of s_sim
// t_coder
// t_sim
// t_sim owns the coder and dongle arrays
// t_coder points back to t_sim
// t_coder points to its two t_dongle objects
// t_dongle owns one request heap
>>>>>>> origin/master
typedef enum e_scheduler
{
	CODEX_FIFO,
	CODEX_EDF
}	t_scheduler;

typedef struct s_config
{
	int			number_of_coders;
	long long	time_to_burnout;
	long long	time_to_compile;
	long long	time_to_debug;
	long long	time_to_refactor;
	int			number_of_compiles_required;
	long long	dongle_cooldown;
	t_scheduler	scheduler;
}	t_config;

typedef struct s_request
{
	int			coder_id;
	long long	sequence;
	long long	deadline;
	int			heap_index;
}	t_request;

typedef struct s_heap
{
	t_request		**items;
	int				size;
	int				capacity;
	t_scheduler		scheduler;
}	t_heap;

<<<<<<< HEAD
int	parse_args(int ac, char **av, t_config *config);
=======
typedef struct  s_dongle
{
	int				id;
	int				hold;
	long long		available_ms;
	pthread_mutex_t	mutex;
	t_heap			queue;
}	t_dongle;

struct s_sim	sim;

typedef struct s_coder
{
	int				id;
	pthread_t		thread;
	struct s_sim	*sim;
	t_dongle		*left;
	t_dongle		*right;
	t_request		left_request;
	t_request		right_request;
	long long		start_of_lastcompile;
	long long		deadline;
	int				compiles_done;
	int				finished;
}	t_coder;

typedef enum e_stop_reason
{
	STOP_NONE,
	STOP_COMPLETED,
	STOP_BURNOUT,
	STOP_ERROR
}	t_stop_reason;

typedef struct s_sim
{
	t_config			config;
	t_coder				*coders;
	t_dongle			*dongles;
	long long			start_time;
	long long			global_sequence;
	pthread_mutex_t		arb_mutex;
	pthread_cond_t		arb_cond;
	pthread_mutex_t		state_mutex;
	pthread_cond_t		monitor_cond;
	t_stop_reason		stop_reason;
	pthread_mutex_t		log_mutex;
	pthread_mutex_t		start_mutex;
	pthread_cond_t		start_cond;
	int					start_released;
	pthread_t			monitor_thread;
}	t_sim;

int	parse_args(int ac, char *av[], t_config *config);
int	request_with_priority(const	t_request *x, const t_request *y, t_scheduler scheduler);
int	heap_init(t_heap *heap, int capacity, t_scheduler scheduler);
int	heap_push(t_heap *heap, t_request *request);
void	shift_down(t_heap *heap, int index);
void	shift_up(t_heap *heap, int index);
t_request	*heap_pop(t_heap *heap);
void	heap_destroy(t_heap *heap);
int	dongle_init(t_dongle *dongle, int id, t_scheduler scheduler);
void	dongle_destroy(t_dongle *dongle);

>>>>>>> origin/master

#endif
