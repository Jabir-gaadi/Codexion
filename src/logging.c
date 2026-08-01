#include "../includes/codexion.h"

int	log_action(t_sim *sim, int coder_id, const char *message)
{
    long long   current_time;
    long long   need_time;

    if (sim == NULL || message == NULL)
        return (0);
    pthread_mutex_lock(&sim->log_mutex);
    pthread_mutex_lock(&sim->state_mutex);
    if (sim->stop_reason != STOP_NONE)
    {
        pthread_mutex_unlock(&sim->state_mutex);
        pthread_mutex_unlock(&sim->log_mutex);
        return (0);
    }
    pthread_mutex_unlock(&sim->state_mutex);
    if (get_time_in_ms(&current_time) == 0)
    {
        pthread_mutex_unlock(&sim->log_mutex);
        return (0);
    }
    need_time = current_time - sim->start_time;
    printf("%lld %d %s\n", need_time, coder_id, message);
    pthread_mutex_unlock(&sim->log_mutex);
    return (1);
}