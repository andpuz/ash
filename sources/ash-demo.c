/// HEADERS

# include <stdbool.h>
# include <stdlib.h>
# include <stdio.h>

# define DS_API

enum ds_event_type {
  DS_EVENT_TYPE_CUSTOM,

  DS_NUM_EVENT_TYPES
};

struct ds_event {
  struct ds_event *             next;
  unsigned int                  time;
  enum ds_event_type            type;
  void *                        data;
};

DS_API bool ds_event_initialize (
  struct ds_event *             self,
  unsigned int                  time,
  enum ds_event_type            type,
  void *                        data
);

DS_API void ds_event_deinitialize (
  struct ds_event *             self
);

DS_API bool ds_event_process (
  struct ds_event *             self
);

DS_API bool ds_event_display (
  struct ds_event *             self,
  FILE *                        file
);

struct ds_event_pool {
  struct ds_event *             events;
  unsigned int                  max_events;
  struct ds_event *             free_event;
};

DS_API bool ds_event_pool_initialize (
  struct ds_event_pool *        self,
  unsigned int                  max_events
);

DS_API void ds_event_pool_deinitialize (
  struct ds_event_pool *        self
);

DS_API struct ds_event * ds_event_pool_acquire (
  struct ds_event_pool *        self,
  unsigned int                  time,
  enum ds_event_type            type,
  void *                        data
);

DS_API void ds_event_pool_release (
  struct ds_event_pool *        self,
  struct ds_event *             event
);

struct ds_event_list {
  struct ds_event *             head;
  struct ds_event *             tail;
};

DS_API void ds_event_list_initialize (
  struct ds_event_list *        self
);

DS_API void ds_event_list_deinitialize (
  struct ds_event_list *        self
);

DS_API void ds_event_list_insert (
  struct ds_event_list *        self,
  struct ds_event *             event
);

DS_API struct ds_event * ds_event_list_remove (
  struct ds_event_list *        self
);

DS_API bool ds_event_list_is_empty (
  struct ds_event_list *        self
);

struct ds_event_bin {
  struct ds_event_bin *         next;
  struct ds_event_list          events;
  unsigned int                  time;
};

DS_API bool ds_event_bin_initialize (
  struct ds_event_bin *         self,
  struct ds_event *             event
);

DS_API void ds_event_bin_deinitialize (
  struct ds_event_bin *         self
);

DS_API void ds_event_bin_insert (
  struct ds_event_bin *         self,
  struct ds_event *             event
);

DS_API struct ds_event * ds_event_bin_remove (
  struct ds_event_bin *         self
);

DS_API bool ds_event_bin_is_empty (
  struct ds_event_bin *         self
);

struct ds_event_bin_pool {
  struct ds_event_bin *         bins;
  unsigned int                  max_bins;
  struct ds_event_bin *         free_bin;
};

DS_API bool ds_event_bin_pool_initialize (
  struct ds_event_bin_pool *    self,
  unsigned int                  max_bins
);

DS_API void ds_event_bin_pool_deinitialize (
  struct ds_event_bin_pool *    self
);

DS_API struct ds_event_bin * ds_event_bin_pool_acquire (
  struct ds_event_bin_pool *    self,
  struct ds_event *             event
);

DS_API void ds_event_bin_pool_release (
  struct ds_event_bin_pool *    self,
  struct ds_event_bin *         bin
);

struct ds_event_queue {
  struct ds_event_pool          events;
  struct ds_event_bin_pool      bins;
  struct ds_event_bin *         head;
  struct ds_event_bin *         tail;
};

DS_API bool ds_event_queue_initialize (
  struct ds_event_queue *       self,
  unsigned int                  max_events,
  unsigned int                  max_bins
);

DS_API void ds_event_queue_deinitialize (
  struct ds_event_queue *       self
);

DS_API bool ds_event_queue_enqueue (
  struct ds_event_queue *       self,
  unsigned int                  time,
  enum ds_event_type            type,
  void *                        data
);

DS_API struct ds_event * ds_event_queue_dequeue (
  struct ds_event_queue *       self,
  unsigned int                  time_limit
);

DS_API void ds_event_queue_recycle (
  struct ds_event_queue *       self,
  struct ds_event *             event
);

DS_API bool ds_event_queue_is_empty (
  struct ds_event_queue *       self
);

struct ds_simulator {
  struct ds_event_queue         queue;
  unsigned int                  time;
  unsigned int                  time_step;
};

DS_API bool ds_simulator_initialize (
  struct ds_simulator *         self,
  unsigned int                  max_events,
  unsigned int                  max_bins,
  unsigned int                  time_step
);

DS_API void ds_simulator_deinitialize (
  struct ds_simulator *         self
);

DS_API bool ds_simulator_schedule (
  struct ds_simulator *         self,
  unsigned int                  time,
  enum ds_event_type            type,
  void *                        data
);

DS_API unsigned int ds_simulator_simulate (
  struct ds_simulator *         self
);

DS_API bool ds_simulator_is_empty (
  struct ds_simulator *         self
);

DS_API unsigned int ds_simulator_drain (
  struct ds_simulator *         self
);

/// MAIN

# include <stdint.h>

int main ( int argc, char const * const * argv )
{
  (void)argc;
  (void)argv;

  struct ds_simulator simulator;

  if ( !ds_simulator_initialize(&simulator, 1024U, 256U, 1U) )
    return EXIT_FAILURE;

  int exit_code = EXIT_SUCCESS;

  for ( int index = 0; index < 10; ++index ) {
    fprintf(stdout, ">>> Scheduling event %d...\n", index);
    bool is_okay  = ds_simulator_schedule(&simulator,
      (unsigned int)( index >> 1U ),
      DS_EVENT_TYPE_CUSTOM,
      (void *)&simulator
    );

    if ( !is_okay ) {
      exit_code = EXIT_FAILURE;
      break;
    }
    fprintf(stdout, "... Scheduled event %d.\n", index);
  }

  if ( EXIT_SUCCESS == exit_code ) {
    unsigned int max_steps  = 1024U * 1024U;
    unsigned int num_steps;

    for ( num_steps = 0; num_steps < max_steps; ++num_steps ) {
      fprintf(stdout, "\n### =========================\n");
      fprintf(stdout, ">>> Processing t=%u, dt=%u...\n",
        simulator.time,
        simulator.time_step
      );
      unsigned int num_events = ds_simulator_simulate(&simulator);
      fprintf(stdout, "... Processed %u events.\n", num_events);
      fprintf(stdout, "### =========================\n");

      if ( ds_simulator_is_empty(&simulator) )
        break;
    }
    fprintf(stdout, "Simulated %u step%s.\nEnd of Simulation.\n",
      num_steps,
      num_steps == 1U ? "" : "s"
    );

    unsigned num_drained_events = ds_simulator_drain(&simulator);
    fprintf(stdout, "%u event%s ha%s been drained.\n",
      num_drained_events,
      num_drained_events == 1U  ? ""  : "s",
      num_drained_events == 1U  ? "s" : "ve"
    );
  }

  ds_simulator_deinitialize(&simulator);
  return exit_code;
}

/// SOURCES

# include <limits.h>
# include <assert.h>
# include <string.h>
# include <errno.h>

# define UNREACHABLE()                                                        \
  do {                                                                        \
    fprintf(stderr, "Unreachable point has been reached!\n");                 \
    abort();                                                                  \
  } while ( false )

# define ERROR(format, ...)     fprintf(stderr, "[ERROR] %s:%d: " format "\n", __FILE__, __LINE__, __VA_ARGS__)
# define ALERT(format, ...)     fprintf(stderr, "[ALERT] %s:%d: " format "\n", __FILE__, __LINE__, __VA_ARGS__)
# define NOTE(format, ...)      fprintf(stderr, "[NOTE ] %s:%d: " format "\n", __FILE__, __LINE__, __VA_ARGS__)
# define INFO(format, ...)      fprintf(stderr, "[INFO ] %s:%d: " format "\n", __FILE__, __LINE__, __VA_ARGS__)
# define DEBUG(format, ...)     fprintf(stderr, "[DEBUG] %s:%d: " format "\n", __FILE__, __LINE__, __VA_ARGS__)
# define TRACE(format, ...)     fprintf(stderr, "[TRACE] %s:%d: " format "\n", __FILE__, __LINE__, __VA_ARGS__)

/// Event

bool ds_event_initialize (
  struct ds_event *             self,
  unsigned int                  time,
  enum ds_event_type            type,
  void *                        data
)
{
  assert(NULL != (void *)self);

  if ( (int)DS_NUM_EVENT_TYPES <= (int)type ) {
    ERROR("Invalid argument `%s`: %s.",
      "type",
      "Out of range [0;DS_NUM_EVENT_TYPES-1]"
    );
    return false;
  }

  self->next  = (struct ds_event *)NULL;
  self->time  = time;
  self->type  = type;
  self->data  = data;

  return true;
}

void ds_event_deinitialize (
  struct ds_event *             self
)
{
  assert(NULL != (void *)self);
  assert(NULL == (void *)self->next);
  assert((int)DS_NUM_EVENT_TYPES > (int)self->type);

  self->next  = (struct ds_event *)NULL;
  self->time  = 0U;
  self->type  = DS_NUM_EVENT_TYPES;
  self->data  = NULL;
}

bool ds_event_process (
  struct ds_event *             self
)
{
  assert(NULL != (void *)self);
  assert(NULL == (void *)self->next);
  assert((int)DS_NUM_EVENT_TYPES > (int)self->type);

  bool is_okay  = ds_event_display(self, stdout);
  assert(is_okay);
  (void)is_okay;

  if ( DS_EVENT_TYPE_CUSTOM == self->type && NULL != self->data ) {
    struct ds_simulator * simulator = (struct ds_simulator *)self->data;

    DEBUG(">>> (Re)Scheduling event <%p>...", self);
    bool is_okay  = ds_simulator_schedule(simulator,
      self->time + 10U,
      self->type,
      self->data
    );
    DEBUG("... %scheduled event <%p> @ %u.",
      is_okay ? "(Re)S" : "Not (re)s",
      self,
      self->time + 10U
    );
  }

  return false;
}

bool ds_event_display (
  struct ds_event *             self,
  FILE *                        file
)
{
  assert(NULL != (void *)self);
  assert(NULL == (void *)self->next);
  assert((int)DS_NUM_EVENT_TYPES > (int)self->type);

  if ( NULL == (void *)file ) {
    ERROR("Invalid argument `%s`: %s.",
      "file",
      "Unexpected null pointer"
    );
    return false;
  }

  static char const * types []  = {
    [ DS_EVENT_TYPE_CUSTOM ]  = "CUSTOM"
  };

  int num_chars = fprintf(file, "Event <%p>: next=<%p> type=%s data=<%p> @ %u\n",
    self,
    self->next,
    types[ (int)self->type ],
    self->data,
    self->time
  );

  return num_chars > 0;
}

/// Event Pool

bool ds_event_pool_initialize (
  struct ds_event_pool *        self,
  unsigned int                  max_events
)
{
  assert(NULL != (void *)self);

  if ( 0U == max_events ) {
    ERROR("Invalid argument `%s`: %s.",
      "max_events",
      "Out of range [1;MAX_UINT]"
    );
    return false;
  }

  struct ds_event * events
    = (struct ds_event *)malloc(
      (size_t)max_events * sizeof(*events)
    );

  if ( NULL == (void *)events ) {
    ERROR("Cannot allocate %u events: %s.",
      max_events,
      strerror(errno)
    );
    return false;
  }

  self->events      = events;
  self->max_events  = max_events;
  self->free_event  = events;

  /// initialize the free list

  events->next  = (struct ds_event *)NULL;

  for ( unsigned int index  = 1U; index < max_events; ++index ) {
    struct ds_event * prev  = events + index - 1U;
    struct ds_event * curr  = events + index;

    prev->next  = curr;
    curr->next  = (struct ds_event *)NULL;
  }

  return true;
}

void ds_event_pool_deinitialize (
  struct ds_event_pool *        self
)
{
  assert(NULL != (void *)self);
  assert(NULL != (void *)self->events);
  assert(0U != self->max_events);

  free(self->events);
}

struct ds_event * ds_event_pool_acquire (
  struct ds_event_pool *        self,
  unsigned int                  time,
  enum ds_event_type            type,
  void *                        data
)
{
  assert(NULL != (void *)self);
  assert(NULL != (void *)self->events);
  assert(0U != self->max_events);

  struct ds_event * event = self->free_event;

  if ( NULL == (void *)event ) {
    ERROR("Out of memory: Maximum number of events (%u) has been reached.",
      self->max_events
    );
    return event;
  }

  struct ds_event * free_event  = event->next;

  bool is_okay  = ds_event_initialize(event,
    time,
    type,
    data
  );

  if ( !is_okay )
    return (struct ds_event *)NULL;

  self->free_event  = free_event;
  return event;
}

void ds_event_pool_release (
  struct ds_event_pool *        self,
  struct ds_event *             event
)
{
  assert(NULL != (void *)self);
  assert(NULL != (void *)self->events);
  assert(0U != self->max_events);

  assert(NULL != (void *)event);
  assert(NULL == (void *)event->next);

  ds_event_deinitialize(event);
  event->next       = self->free_event;
  self->free_event  = event;
}

/// Event List

void ds_event_list_initialize (
  struct ds_event_list *        self
)
{
  assert(NULL != (void *)self);

  self->head = (struct ds_event *)NULL;
  self->tail = (struct ds_event *)NULL;
}

void ds_event_list_deinitialize (
  struct ds_event_list *        self
)
{
  assert(NULL != (void *)self);
  assert(NULL == (void *)self->head);
  assert(NULL == (void *)self->tail);

  (void)self;
}

void ds_event_list_insert (
  struct ds_event_list *        self,
  struct ds_event *             event
)
{
  assert(NULL != (void *)self);
  assert(NULL != (void *)event);
  assert(NULL == (void *)event->next);

  if ( NULL == (void *)self->head ) {
    self->head  = event;
    self->tail  = event;
  } else {
    self->tail->next  = event;
    self->tail  = event;
  }
}

struct ds_event * ds_event_list_remove (
  struct ds_event_list *        self
)
{
  assert(NULL != (void *)self);

  struct ds_event * event = self->head;

  if ( NULL == (void *)event )
    return event;

  self->head  = event->next;
  event->next = (struct ds_event *)NULL;

  if ( NULL == (void *)self->head ) {
    self->tail  = self->head;
  }

  return event;
}

bool ds_event_list_is_empty (
  struct ds_event_list *        self
)
{
  assert(NULL != (void *)self);

  return NULL == (void *)self->head;
}

/// Event Bin

bool ds_event_bin_initialize (
  struct ds_event_bin *         self,
  struct ds_event *             event
)
{
  assert(NULL != (void *)self);

  if ( NULL == (void *)event ) {
    ERROR("Invalid argument `%s`: %s.",
      "event",
      "Unexpected null pointer"
    );
    return false;
  }

  ds_event_list_initialize(&self->events);
  self->next  = (struct ds_event_bin *)NULL;
  self->time  = event->time;

  /// insert the first event
  ds_event_list_insert(&self->events, event);

  return true;
}

void ds_event_bin_deinitialize (
  struct ds_event_bin *         self
)
{
  assert(NULL != (void *)self);
  assert(NULL == (void *)self->next);

  ds_event_list_deinitialize(&self->events);
}

void ds_event_bin_insert (
  struct ds_event_bin *         self,
  struct ds_event *             event
)
{
  assert(NULL != (void *)self);

  ds_event_list_insert(&self->events, event);
}

struct ds_event * ds_event_bin_remove (
  struct ds_event_bin *         self
)
{
  assert(NULL != (void *)self);

  return ds_event_list_remove(&self->events);
}

bool ds_event_bin_is_empty (
  struct ds_event_bin *         self
)
{
  assert(NULL != (void *)self);

  return ds_event_list_is_empty(&self->events);
}

/// Event Bin Pool

bool ds_event_bin_pool_initialize (
  struct ds_event_bin_pool *    self,
  unsigned int                  max_bins
)
{
  assert(NULL != (void *)self);

  if ( 0U == max_bins ) {
    ERROR("Invalid argument `%s`: %s.",
      "max_bins",
      "Out of range [1;MAX_UINT]"
    );
    return false;
  }

  struct ds_event_bin * bins
    = (struct ds_event_bin *)malloc(
      (size_t)max_bins * sizeof(*bins)
    );

  if ( NULL == (void *)bins ) {
    ERROR("Cannot allocate %u bins: %s.",
      max_bins,
      strerror(errno)
    );
    return false;
  }

  self->bins      = bins;
  self->max_bins  = max_bins;
  self->free_bin  = bins;

  /// initialize the free list

  bins->next  = (struct ds_event_bin *)NULL;

  for ( unsigned int index  = 1U; index < max_bins; ++index ) {
    struct ds_event_bin * prev  = bins + index - 1U;
    struct ds_event_bin * curr  = bins + index;

    prev->next  = curr;
    curr->next  = (struct ds_event_bin *)NULL;
  }

  return true;
}

void ds_event_bin_pool_deinitialize (
  struct ds_event_bin_pool *    self
)
{
  assert(NULL != (void *)self);
  assert(NULL != (void *)self->bins);
  assert(0U != self->max_bins);

  free(self->bins);
}

struct ds_event_bin * ds_event_bin_pool_acquire (
  struct ds_event_bin_pool *    self,
  struct ds_event *             event
)
{
  assert(NULL != (void *)self);
  assert(NULL != (void *)self->bins);
  assert(0U != self->max_bins);

  struct ds_event_bin * bin = self->free_bin;

  if ( NULL == (void *)bin ) {
    ERROR("Out of memory: Maximum number of event bins (%u) has been reached.",
      self->max_bins
    );
    return bin;
  }

  struct ds_event_bin * free_bin  = bin->next;

  bool is_okay  = ds_event_bin_initialize(bin, event);

  if ( !is_okay )
    return (struct ds_event_bin *)NULL;

  self->free_bin  = free_bin;
  return bin;
}

void ds_event_bin_pool_release (
  struct ds_event_bin_pool *    self,
  struct ds_event_bin *         bin
)
{
  assert(NULL != (void *)self);
  assert(NULL != (void *)self->bins);
  assert(0U != self->max_bins);

  assert(NULL != (void *)bin);
  assert(NULL == (void *)bin->next);

  ds_event_bin_deinitialize(bin);
  bin->next       = self->free_bin;
  self->free_bin  = bin;
}

/// Event Queue

bool ds_event_queue_initialize (
  struct ds_event_queue *       self,
  unsigned int                  max_events,
  unsigned int                  max_bins
)
{
  assert(NULL != (void *)self);

  bool is_okay;

  is_okay = ds_event_pool_initialize(&self->events, max_events);

  if ( !is_okay )
    return is_okay;

  is_okay = ds_event_bin_pool_initialize(&self->bins, max_bins);

  if ( !is_okay ) {
    ds_event_pool_deinitialize(&self->events);
    return is_okay;
  }

  self->head  = (struct ds_event_bin *)NULL;
  self->tail  = (struct ds_event_bin *)NULL;

  return true;
}

void ds_event_queue_deinitialize (
  struct ds_event_queue *       self
)
{
  assert(NULL != (void *)self);
  assert(NULL == (void *)self->head);
  assert(NULL == (void *)self->tail);

  ds_event_bin_pool_deinitialize(&self->bins);
  ds_event_pool_deinitialize(&self->events);
}

bool ds_event_queue_enqueue (
  struct ds_event_queue *       self,
  unsigned int                  time,
  enum ds_event_type            type,
  void *                        data
)
{
  assert(NULL != (void *)self);

  struct ds_event * event = ds_event_pool_acquire(&self->events,
    time,
    type,
    data
  );

  if ( NULL == (void *)event )
    return false;

  struct ds_event_bin * curr  = self->head;
  struct ds_event_bin * prev;

  while ( NULL != (void *)curr ) {
    if ( time <= curr->time )
      break;

    prev  = curr;
    curr  = curr->next;
  }

  if ( NULL == (void *)curr || time != curr->time ) {
    /// no bin has been found, then acquire a new one
    struct ds_event_bin * bin = ds_event_bin_pool_acquire(&self->bins, event);

    if ( NULL == (void *)bin ) {
      ds_event_pool_release(&self->events, event);
      return false;
    }

    if ( NULL != (void *)curr ) {
      prev->next  = bin;
      bin->next   = curr;
      return true;
    }

    /// append the bin
    if ( NULL != (void *)self->tail ) {
      self->tail->next  = bin;
    }

    self->tail  = bin;

    if ( NULL == (void *)self->head ) {
      self->head  = self->tail;
    }

    return true;
  }

  ds_event_bin_insert(curr, event);
  return true;
}

struct ds_event * ds_event_queue_dequeue (
  struct ds_event_queue *       self,
  unsigned int                  time_limit
)
{
  assert(NULL != (void *)self);

  struct ds_event_bin * bin = self->head;

  if ( NULL == (void *)bin || time_limit <= bin->time )
    return (struct ds_event *)NULL;

  struct ds_event * event = ds_event_bin_remove(bin);
  /// if the bin is present, it cannot be empty
  assert(NULL != (void *)event);

  if ( ds_event_bin_is_empty(bin) ) {
    self->head  = bin->next;

    if ( NULL == (void *)self->head ) {
      self->tail  = self->head;
    }

    bin->next = (struct ds_event_bin *)NULL;
    ds_event_bin_pool_release(&self->bins, bin);
  }

  return event;
}

void ds_event_queue_recycle (
  struct ds_event_queue *       self,
  struct ds_event *             event
)
{
  assert(NULL != (void *)self);
  assert(NULL != (void *)event);

  ds_event_pool_release(&self->events, event);
}

bool ds_event_queue_is_empty (
  struct ds_event_queue *       self
)
{
  assert(NULL != (void *)self);

  return NULL == (void *)self->head;
}

/// Simulator

bool ds_simulator_initialize (
  struct ds_simulator *         self,
  unsigned int                  max_events,
  unsigned int                  max_bins,
  unsigned int                  time_step
)
{
  assert(NULL != (void *)self);

  if ( 0U == time_step ) {
    ERROR("Invalid argument `%s`: %s.",
      "time_step",
      "Out of range [1;MAX_UINT]"
    );
    return false;
  }

  bool is_okay;

  is_okay = ds_event_queue_initialize(&self->queue, max_events, max_bins);

  if ( !is_okay )
    return is_okay;

  self->time      = 0U;
  self->time_step = time_step;

  return true;
}

void ds_simulator_deinitialize (
  struct ds_simulator *         self
)
{
  assert(NULL != (void *)self);
  assert(0U != self->time_step);

  ds_event_queue_deinitialize(&self->queue);
}

bool ds_simulator_schedule (
  struct ds_simulator *         self,
  unsigned int                  time,
  enum ds_event_type            type,
  void *                        data
)
{
  assert(NULL != (void *)self);

  if ( time < self->time ) {
    ERROR("Invalid argument `%s`: Scheduling time %u has to be >=%u.",
      "time",
      time,
      self->time
    );
    return false;
  }

  return ds_event_queue_enqueue(&self->queue,
    time,
    type,
    data
  );
}

unsigned int ds_simulator_simulate (
  struct ds_simulator *         self
)
{
  assert(NULL != (void *)self);

  unsigned int num_events = 0U;
  unsigned int time_limit = self->time + self->time_step;

  do {
    struct ds_event * event = ds_event_queue_dequeue(&self->queue, time_limit);

    if ( NULL == (void *)event )
      break;

    ds_event_process(event);
    ++num_events;

    ds_event_queue_recycle(&self->queue, event);
  } while ( true );

  self->time += self->time_step;

  return num_events;
}

bool ds_simulator_is_empty (
  struct ds_simulator *         self
)
{
  assert(NULL != (void *)self);

  return ds_event_queue_is_empty(&self->queue);
}

unsigned int ds_simulator_drain (
  struct ds_simulator *         self
)
{
  assert(NULL != (void *)self);

  unsigned int num_events = 0U;

  do {
    struct ds_event * event = ds_event_queue_dequeue(&self->queue, UINT_MAX);

    if ( NULL == (void *)event )
      break;

    ++num_events;

    ds_event_queue_recycle(&self->queue, event);
  } while ( true );

  return num_events;
}
