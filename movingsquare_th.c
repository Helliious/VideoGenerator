/* @file movingsquare_th.c
*
* This File contains a program that creates a video which moves a square
* around the screen with threads. The square bounces from the screen walls
* and every time it bounces, it changes its color and its direction.
*
*/
/* --------------------------------------------------------------------------
*/


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

#define SQUARE_WIDTH 120
#define SQUARE_HEIGHT 120

typedef struct {
    int begin_square_x;
    int begin_square_y;
    int end_square_x;
    int end_square_y;
    int size_square;
    int frame_step;
    int color_l;
    int color_chb;
    int color_chr;
} square;

typedef struct {
    char *buf;
    char *help_buf;
    int begin_x;
    int begin_y;
} frame;

typedef struct {
    frame *frm;
    square sqr;
    FILE *f;
    sem_t producer_lock;
    sem_t consumer_lock;
    int buff_count;
    int size_buf;
    int producer_buff_index;
    int consumer_buff_index;
} config;

/* ==========================================================================
*/
/*
* Function create_cfg configurates the structures and sets the background
* color.
*
* @cfg variable of type config.
*/
/* ==========================================================================
*/

int create_cfg (config *cfg)
{
    int i;

/** opening the file descriptor */
    cfg->f = fopen ("file", "w");

    if (!cfg->f) {
        printf ("Couldn't open the file!\n");
        return 1;
    }

    fprintf (cfg->f,"YUV4MPEG2 W1920 H1080 F30:1\n");

    cfg->size_buf = (SCREEN_WIDTH * SCREEN_HEIGHT) +
                (SCREEN_WIDTH * SCREEN_HEIGHT) / 2;

    cfg->sqr.begin_square_x = 0;
    cfg->sqr.begin_square_y = 0;
    cfg->sqr.end_square_x = 0;
    cfg->sqr.end_square_y = 0;
    cfg->sqr.color_l = 0;
    cfg->sqr.color_chb = 0;
    cfg->sqr.color_chr = 0;

    cfg->producer_buff_index = cfg->buff_count;
    cfg->consumer_buff_index = cfg->buff_count;

    cfg->sqr.size_square = (SQUARE_WIDTH * SQUARE_HEIGHT);

    frame *tmp = (frame *) calloc (cfg->buff_count, sizeof (frame));
    cfg->frm = tmp;

    char *background_tmp = (char *) calloc (cfg->size_buf, sizeof (char));

    char *w_background_tmp = background_tmp;

/** setting the background's luma */
    for (i = 0 ; i < (SCREEN_WIDTH * SCREEN_HEIGHT) ; i++) {
        *w_background_tmp++ = rand () % 256;
    }

/** setting the background's chromas */
    memset (w_background_tmp, 0x80,
        ((SCREEN_WIDTH * SCREEN_HEIGHT) / 2) * sizeof (char));

    for (i = 0 ; i < cfg->buff_count ; i++, tmp++) {
        tmp->buf = (char *) calloc (cfg->size_buf, sizeof (char));
        tmp->help_buf = (char *) calloc (cfg->sqr.size_square,
                                        sizeof (char));
        memcpy (tmp->buf, background_tmp, cfg->size_buf * sizeof (char));
        tmp->begin_x = 0;
        tmp->begin_y = 0;
    }

    sem_init (&cfg->producer_lock, 1, cfg->buff_count - 1);
    sem_init (&cfg->consumer_lock, 1, 0);

    free (background_tmp);

    return 0;
}

/* ==========================================================================
*/
/*
* Function destroy_cfg frees the allocated memory and closes the file
* descriptor.
*
* @cfg variable of type config.
*/
/* ==========================================================================
*/

int destroy_cfg (config *cfg)
{
    int i;
    frame *w_frm = cfg->frm;

    for (i = 0 ; i < cfg->buff_count ; i++, w_frm++) {
        free (w_frm->buf);
        free (w_frm->help_buf);
    }

    free (cfg->frm);

    fclose (cfg->f);

    return 0;
}

/* ==========================================================================
*/
/*
* Function save_background keeps the background under the square.
*
* @frm variable of type frame.
*
* @sqr variable of type square.
*/
/* ==========================================================================
*/

void save_background (frame *frm,
                    square *sqr)
{
    int i, j;
    char *luma = frm->buf;
    char *help_buf = frm->help_buf;

    frm->begin_x = sqr->begin_square_x;
    frm->begin_y = sqr->begin_square_y;

    luma += (sqr->begin_square_y) * SCREEN_WIDTH;
    for (i = 0 ; i < SQUARE_HEIGHT ; i++) {
        luma += sqr->begin_square_x;

        for (j = 0 ; j < SQUARE_WIDTH ; j++) {
            *help_buf++ = *luma++;
        }

        luma += (SCREEN_WIDTH - (sqr->begin_square_x + SQUARE_WIDTH));
    }
}

/* ==========================================================================
*/
/*
* Function reload_background returns the piece of the background which was
* under the square.
*
* @frm variable of type frame.
*
* @sqr variable of type square.
*/
/* ==========================================================================
*/

void reload_background (frame *frm,
                        square *sqr)
{
    int i, j;
    char *luma = &frm->buf[0];
    char *chroma_b = &frm->buf[SCREEN_WIDTH * SCREEN_HEIGHT];
    char *chroma_r = &frm->buf[(SCREEN_WIDTH * SCREEN_HEIGHT) +
                        (SCREEN_WIDTH * SCREEN_HEIGHT) / 4];
    char *help_buf = &frm->help_buf[0];

    luma += (frm->begin_y) * SCREEN_WIDTH;

    for (i = 0 ; i < SQUARE_HEIGHT ; i++) {
        luma += frm->begin_x;

        for (j = 0 ; j < SQUARE_WIDTH ; j++) {
            *luma++ = *help_buf++;
        }

        luma += (SCREEN_WIDTH - (frm->begin_x + SQUARE_WIDTH));
    }

    chroma_b += ((frm->begin_y) / 2) * (SCREEN_WIDTH / 2);
    chroma_r += ((frm->begin_y) / 2) * (SCREEN_WIDTH / 2);

    for (i = SQUARE_HEIGHT / 2 ; i != 0 ; i--) {
        chroma_b += frm->begin_x / 2;
        chroma_r += frm->begin_x / 2;

        for (j = SQUARE_WIDTH / 2 ; j != 0 ; j--) {
            *chroma_b++ = 0x80;
            *chroma_r++ = 0x80;
        }

        chroma_b += (SCREEN_WIDTH / 2) -
                    ((frm->begin_x + SQUARE_WIDTH) / 2);
        chroma_r += (SCREEN_WIDTH / 2) -
                    ((frm->begin_x + SQUARE_WIDTH) / 2);
    }
}

/* ==========================================================================
*/
/*
* Function create_square creates a square on our screen and sets it to a
* specified color.
*
* @frm variable of type frame.
*
* @sqr variable of type square.
*/
/* ==========================================================================
*/

void create_square (frame *frm,
                    square *sqr)
{
    int i, j;
    char *luma = &frm->buf[0];
    char *chroma_b = &frm->buf[SCREEN_WIDTH * SCREEN_HEIGHT];
    char *chroma_r = &frm->buf[(SCREEN_WIDTH * SCREEN_HEIGHT) +
                        (SCREEN_WIDTH * SCREEN_HEIGHT) / 4];

    luma += (sqr->begin_square_y) * SCREEN_WIDTH;

    for (i = sqr->begin_square_y ;
        i < sqr->end_square_y ;
        i++) {
        luma += sqr->begin_square_x;

        for (j = sqr->begin_square_x ;
            j < sqr->end_square_x ;
            j++) {
            *luma++ = sqr->color_l;
        }

        luma += (SCREEN_WIDTH - (sqr->begin_square_x + SQUARE_WIDTH));
    }

    chroma_b += ((sqr->begin_square_y) / 2) * (SCREEN_WIDTH / 2);
    chroma_r += ((sqr->begin_square_y) / 2) * (SCREEN_WIDTH / 2);

    for (i = sqr->begin_square_y / 2 ;
        i < sqr->end_square_y / 2 ;
        i++) {
        chroma_b += sqr->begin_square_x / 2;
        chroma_r += sqr->begin_square_x / 2;

        for (j = sqr->begin_square_x / 2 ;
            j < sqr->end_square_x / 2 ;
            j++) {
            *chroma_b++ = sqr->color_chb;
            *chroma_r++ = sqr->color_chr;
        }

        chroma_b += (SCREEN_WIDTH / 2) -
                    ((sqr->begin_square_x + SQUARE_WIDTH) / 2);
        chroma_r += (SCREEN_WIDTH / 2) -
                    ((sqr->begin_square_x + SQUARE_WIDTH) / 2);
    }
}

/* ==========================================================================
*/
/*
* Function choose_frame gives a chosen frame to the producer or consumer
* depending on the flag. If the producer or consumer index is over the
* limit we have set we reset them to 0.
*
* @frm variable of type frame.
*
* @sqr variable of type square.
*/
/* ==========================================================================
*/

frame *choose_frame (config *cfg, int flag)
{
/** static working pointer */
    static frame *w_frm_producer;
/** static working pointer */
    static frame *w_frm_consumer;

    if (flag == 0) {
        if (cfg->producer_buff_index >= cfg->buff_count-1) {
            w_frm_producer = cfg->frm;
            cfg->producer_buff_index = 0;
        } else {
            w_frm_producer++;
            cfg->producer_buff_index++;
        }

        return w_frm_producer;

    } else if (flag == 1) {
        if (cfg->consumer_buff_index >= cfg->buff_count-1) {
            w_frm_consumer = cfg->frm;
            cfg->consumer_buff_index = 0;
        } else {
            w_frm_consumer++;
            cfg->consumer_buff_index++;
        }

        return w_frm_consumer;
    }

    return NULL;
}

/* ==========================================================================
*/
/*
* Function move_square_down_right creates a set of frames which are stored
* in a file and then executed by a media player. This function moves the
* square with the help of many frames which are storing different positions
* of our square.
*
* @cfg variable of type config.
*/
/* ==========================================================================
*/

void move_square_down_right (config *cfg)
{
/** working pointer */
    frame *w_frm;

/** luminance color of square */
    cfg->sqr.color_l = (rand () % 256);
/** chroma_b color of square */
    cfg->sqr.color_chb = (rand () % 256);
/** chroma_r color of square */
    cfg->sqr.color_chr = (rand () % 256);

/** this for cycle is for moving down right the y and x coordinates */
    for ( ;
        (cfg->sqr.begin_square_x <= SCREEN_WIDTH - SQUARE_WIDTH) &&
        (cfg->sqr.begin_square_y <= SCREEN_HEIGHT - SQUARE_HEIGHT) ;
        cfg->sqr.begin_square_x += cfg->sqr.frame_step,
        cfg->sqr.begin_square_y += cfg->sqr.frame_step) {
        cfg->sqr.end_square_x = cfg->sqr.begin_square_x + SQUARE_WIDTH;
        cfg->sqr.end_square_y = cfg->sqr.begin_square_y + SQUARE_HEIGHT;

        w_frm = choose_frame (cfg, 0);

        sem_wait (&cfg->producer_lock);
       	save_background (w_frm, &cfg->sqr);

        create_square (w_frm, &cfg->sqr);

        sem_post (&cfg->consumer_lock);

    }

    cfg->sqr.begin_square_x -= cfg->sqr.frame_step;
    cfg->sqr.begin_square_y -= cfg->sqr.frame_step;
}

/* ==========================================================================
*/
/*
* Function move_square_down_left creates a set of frames which are stored
* in a file and then executed by a media player. This function moves the
* square with the help of many frames which are storing different positions
* of our square.
*
* @cfg variable of type config.
*/
/* ==========================================================================
*/

void move_square_down_left (config *cfg)
{
/** working pointer */
    frame *w_frm;

/** luminance color of square */
    cfg->sqr.color_l = (rand () % 256);
/** chroma_b color of square */
    cfg->sqr.color_chb = (rand () % 256);
/** chroma_r color of square */
    cfg->sqr.color_chr = (rand () % 256);

/** this for cycle is for moving down left the y and x coordinates */
    for ( ;
        (cfg->sqr.begin_square_y <= SCREEN_HEIGHT - SQUARE_HEIGHT) &&
        (cfg->sqr.begin_square_x >= 0) ;
        cfg->sqr.begin_square_x -= cfg->sqr.frame_step,
        cfg->sqr.begin_square_y += cfg->sqr.frame_step) {
        cfg->sqr.end_square_x = cfg->sqr.begin_square_x + SQUARE_WIDTH;
        cfg->sqr.end_square_y = cfg->sqr.begin_square_y + SQUARE_HEIGHT;

        w_frm = choose_frame (cfg, 0);

        sem_wait (&cfg->producer_lock);
        save_background (w_frm, &cfg->sqr);

        create_square (w_frm, &cfg->sqr);

        sem_post (&cfg->consumer_lock);
    }

    cfg->sqr.begin_square_x += cfg->sqr.frame_step;
    cfg->sqr.begin_square_y -= cfg->sqr.frame_step;
}

/* ==========================================================================
*/
/*
* Function move_square_up_right creates a set of frames which are stored
* in a file and then executed by a media player. This function moves the
* square with the help of many frames which are storing different positions
* of our square.
*
* @cfg variable of type config.
*/
/* ==========================================================================
*/

void move_square_up_right (config *cfg)
{
/** working pointer */
    frame *w_frm;

/** luminance color of square */
    cfg->sqr.color_l = (rand () % 256);
/** chroma_b color of square */
    cfg->sqr.color_chb = (rand () % 256);
/** chroma_r color of square */
    cfg->sqr.color_chr = (rand () % 256);

/** this for cycle is for moving up right the y and x coordinates */
    for ( ;
        (cfg->sqr.begin_square_x <= SCREEN_WIDTH - SQUARE_WIDTH) &&
        (cfg->sqr.begin_square_y >= 0) ;
        cfg->sqr.begin_square_x += cfg->sqr.frame_step,
        cfg->sqr.begin_square_y -= cfg->sqr.frame_step) {
        cfg->sqr.end_square_x = cfg->sqr.begin_square_x + SQUARE_WIDTH;
        cfg->sqr.end_square_y = cfg->sqr.begin_square_y + SQUARE_HEIGHT;

        w_frm = choose_frame (cfg, 0);

        sem_wait (&cfg->producer_lock);
        save_background (w_frm, &cfg->sqr);

        create_square (w_frm, &cfg->sqr);

        sem_post (&cfg->consumer_lock);
    }

    cfg->sqr.begin_square_x -= cfg->sqr.frame_step;
    cfg->sqr.begin_square_y += cfg->sqr.frame_step;
}

/* ==========================================================================
*/
/*
* Function move_square_up_left creates a set of frames which are stored
* in a file and then executed by a media player. This function moves the
* square with the help of many frames which are storing different positions
* of our square.
*
* @cfg variable of type config.
*/
/* ==========================================================================
*/

void move_square_up_left (config *cfg)
{
/** working pointer */
    frame *w_frm;

/** luminance color of square */
    cfg->sqr.color_l = (rand () % 256);
/** chroma_b color of square */
    cfg->sqr.color_chb = (rand () % 256);
/** chroma_r color of square */
    cfg->sqr.color_chr = (rand () % 256);

/** this for cycle is for moving up left the y and x coordinates */
    for ( ;
        cfg->sqr.begin_square_x >= 0 &&
        cfg->sqr.begin_square_y >= 0 ;
        cfg->sqr.begin_square_x -= cfg->sqr.frame_step,
        cfg->sqr.begin_square_y -= cfg->sqr.frame_step) {
        cfg->sqr.end_square_x = cfg->sqr.begin_square_x + SQUARE_WIDTH;
        cfg->sqr.end_square_y = cfg->sqr.begin_square_y + SQUARE_HEIGHT;

        w_frm = choose_frame (cfg, 0);

        sem_wait (&cfg->producer_lock);
        save_background (w_frm, &cfg->sqr);

        create_square (w_frm, &cfg->sqr);

        sem_post (&cfg->consumer_lock);
    }

    cfg->sqr.begin_square_x += cfg->sqr.frame_step;
    cfg->sqr.begin_square_y += cfg->sqr.frame_step;
}

/* ==========================================================================
*/
/*
* Function producer holds and controls the four functions for the movement
* of the square. This function decides which direction to be next and when
* to change the direction.
*
* @cfg variable of type config.
*/
/* ==========================================================================
*/

int producer (config *cfg)
{
/** sign we use to know to go up or down */
    int sign_y = 1;
/** sign we use to know to left or right */
    int sign_x = 1;

    while (cfg->sqr.begin_square_y >= -120) {
        if (sign_x == 1 && sign_y == 1) {
            move_square_down_right (cfg);
        } else if (sign_x == 1 && sign_y == -1) {
            move_square_up_right (cfg);
        } else if (sign_x == -1 && sign_y == -1) {
            move_square_up_left (cfg);
        } else if (sign_x == -1 && sign_y == 1) {
            move_square_down_left (cfg);
        }

/** the case when we will change the y direction of the square */
        if ((cfg->sqr.begin_square_y >= SCREEN_HEIGHT - SQUARE_HEIGHT) ||
            (cfg->sqr.begin_square_y <= 0)) {
            sign_y = -sign_y;
        }

/** the case when we will change the x direction of the square */
        if ((cfg->sqr.begin_square_x >= SCREEN_WIDTH -
            SQUARE_WIDTH - cfg->sqr.frame_step) ||
            (cfg->sqr.begin_square_x <= 0)) {
            sign_x = -sign_x;
        }
    }

    return 0;
}

/* ==========================================================================
*/
/*
* Function consumer is the function which takes a chosen frame and writes
* it to a file which will be then executed as a video.
*
* @cfg variable of type config.
*/
/* ==========================================================================
*/

int consumer (config *cfg)
{
/** number of frames we want to consume */
    int frames = 2000;
/** working pointer */
    frame *w_frm;

    while (frames--) {
        printf("frames: %d\n", frames);
        w_frm = choose_frame (cfg, 1);

        sem_wait (&cfg->consumer_lock);

        fprintf (cfg->f, "FRAME\n");
        fwrite (w_frm->buf, sizeof (char), cfg->size_buf, cfg->f);

        reload_background (w_frm, &cfg->sqr);

        sem_post (&cfg->producer_lock);
    }

    return 0;
}

/* ==========================================================================
*/
/*
* In main function we set the frame_step and we call the create_all function,
* which we give the parameters we want for our screen width and height and
* our frame_step.
*
* @argc holds the number of strings pointed to by argv.
*
* @argv we don't use it in the current program.
*/
/* ==========================================================================
*/


int main (int argc, char *argv[])
{
/** variable of type config */
    config cfg;
/** frame step we want, the speed of the square */
    cfg.sqr.frame_step = 15;
/** number of buffers we want to use for the two threads */
    cfg.buff_count = 10;
/** ids for the two threads */
    pthread_t *pth1, *pth2;

    pth1 = (pthread_t *)calloc (1, sizeof (*pth1));
    pth2 = (pthread_t *)calloc (1, sizeof (*pth2));

    while ((SCREEN_HEIGHT - SQUARE_WIDTH) % cfg.sqr.frame_step != 0) {
        cfg.sqr.frame_step++;
    }

    create_cfg (&cfg);

    pthread_create (pth1, NULL, (void *) producer, &cfg);
    pthread_create (pth2, NULL, (void *) consumer, &cfg);


    pthread_join (*pth1, NULL);
    pthread_join (*pth2, NULL);

    destroy_cfg (&cfg);

    return 0;
}