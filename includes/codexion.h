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
# include <stdint.h>
# include <math.h>
# include <unistd.h>
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
	t_request		*items[2];
	int				size;
	t_scheduler		scheduler;
}	t_heap;

typedef struct s_dongle
{
	int				id;
	int				hold;
	long long		available_ms;
	pthread_mutex_t	mutex;
	t_heap			queue;
}	t_dongle;

struct	s_sim;

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

int			parse_args(int ac, char *av[], t_config *config);
int			get_time_in_ms(long long *result);
int			add_time_ms(long long base, long long duration, long long *result);
int			ms_to_timespec(long long ms_time, struct timespec *result);
int			request_with_priority(const	t_request *x,
				const t_request *y, t_scheduler scheduler);
int			heap_init(t_heap *heap, int capacity, t_scheduler scheduler);
int			heap_push(t_heap *heap, t_request *request);
t_request	*heap_peek(t_heap *heap);
int			heap_remove(t_heap *heap, t_request *request);
t_request	*heap_pop(t_heap *heap);
void		heap_destroy(t_heap *heap);
int			dongle_init(t_dongle *dongle, int id, t_scheduler scheduler);
int			release_dongles_basic(t_coder *coder);
void		dongle_destroy(t_dongle *dongle);
void		*coder_routine(void *args);
void		*monitor_routine(void *args);
int			sim_init(t_sim *sim, const t_config *config);
void		init_coder(t_sim *sim, int index);
int			sim_release_start(t_sim *sim);
void		sim_destroy(t_sim *sim);
int			sim_start_threads(t_sim *sim);
void		wait_for_start(t_sim *sim);
int			sim_release_start(t_sim *sim);
int			sim_join_threads(t_sim *sim);
int			sim_request_stop(t_sim *sim, t_stop_reason reason,
				int coder_id, long long terminal_time);
int			sim_sleep(t_sim	*sim, long long duration);
int			log_action(t_sim *sim, int coder_id, const char *message);
int			take_dongles_basic(t_coder *coder);
int			sim_is_running(t_sim *sim);
void		init_sim_or_destroy(t_sim *sim, int select);
void		join_coders_threads(t_sim *sim, int total_thread);
int			is_my_turn(t_dongle *first, t_dongle *second, t_request **arr_req,
				long long current_time);
void		claim_dongles(t_dongle *first, t_dongle *second);
void		cancel_reqeust(t_dongle *first, t_dongle *second,
				t_request *first_req,
				t_request *second_req);

#endif
