#include <stdlib.h>
#include <stdio.h>
#include <clutter/clutter.h>
#include <clutter/cogl.h>
/*#include <GL/glx.h>*/
#include <GL/glu.h>

const gint NO_BOXES_X=2;
const gint NO_BOXES_Y=2;

struct BoxProperties {
    ClutterKnot      pathBegin;
    ClutterActor     *box;
    ClutterTimeline  *timeline;
    ClutterBehaviour *rotY;
    ClutterBehaviour *rotZ;
    ClutterBehaviour *transZ;
    ClutterBehaviour *transXY;
};

void cleanupBoxProperties( struct BoxProperties *this )
{
  printf( "cleaning up\n" );
  g_object_unref( this->rotZ );
  g_object_unref( this->rotY );
  g_object_unref( this->transZ );
  g_object_unref( this->transXY );
  g_object_unref( this->timeline );
};

static gboolean
on_button_release( ClutterActor *rect, ClutterEvent *event, gpointer data )
{
  ClutterTimeline  *timeline = (ClutterTimeline*)data;
  gint x = 0, y = 0;
  clutter_event_get_coords( event, &x, &y );

  clutter_timeline_start( timeline );

  g_print( "Click-release on rectangle at (%d, %d)\n", x, y );

  return TRUE; /* Stop further handling of this event. */
}

static ClutterActor *
clone_box( ClutterTexture *original, guint width, guint height, guint depth )
{
  ClutterActor *group  = clutter_group_new();

  /* clone texture */
  ClutterActor *front  = clutter_clone_texture_new( original );
  ClutterActor *back   = clutter_clone_texture_new( original );
  ClutterActor *left   = clutter_clone_texture_new( original );
  ClutterActor *right  = clutter_clone_texture_new( original );
  ClutterActor *bottom = clutter_clone_texture_new( original );
  ClutterActor *top    = clutter_clone_texture_new( original );

  /* add each face to the group */
  clutter_container_add_actor( CLUTTER_CONTAINER (group), front );
  clutter_container_add_actor( CLUTTER_CONTAINER (group), back );
  clutter_container_add_actor( CLUTTER_CONTAINER (group), left );
  clutter_container_add_actor( CLUTTER_CONTAINER (group), right );
  clutter_container_add_actor( CLUTTER_CONTAINER (group), bottom );
  clutter_container_add_actor( CLUTTER_CONTAINER (group), top );

  /* move anchor point */
  clutter_actor_set_anchor_point( front,  width/2, height/2 );
  clutter_actor_set_anchor_point( back,   width/2, height/2 );
  clutter_actor_set_anchor_point( left,   depth/2, height/2 );
  clutter_actor_set_anchor_point( right,  depth/2, height/2 );
  clutter_actor_set_anchor_point( bottom, width/2, depth/2 );
  clutter_actor_set_anchor_point( top,    width/2, depth/2 );

  /* set size */
  clutter_actor_set_size( front,  width, height );
  clutter_actor_set_size( back,   width, height );
  clutter_actor_set_size( left,   depth, height );
  clutter_actor_set_size( right,  depth, height );
  clutter_actor_set_size( bottom, width, depth );
  clutter_actor_set_size( top,    width, depth );

  /* orient each face appropriately */
  clutter_actor_set_rotation( front,  CLUTTER_Y_AXIS,    0, 0, 0, 0 );
  clutter_actor_set_rotation( back,   CLUTTER_Y_AXIS, -180, 0, 0, 0 );
  clutter_actor_set_rotation( left,   CLUTTER_Y_AXIS,   90, 0, 0, 0 );
  clutter_actor_set_rotation( right,  CLUTTER_Y_AXIS,  -90, 0, 0, 0 );
  clutter_actor_set_rotation( bottom, CLUTTER_X_AXIS,   90, 0, 0, 0 );
  clutter_actor_set_rotation( top,    CLUTTER_X_AXIS,  -90, 0, 0, 0 );

  /* move them to the correct distance from the origin */
  clutter_actor_set_depth( front, depth/2 );
  clutter_actor_set_depth( back, -depth/2 );
  clutter_actor_set_position( left,   width/2, 0 );
  clutter_actor_set_position( right, -width/2, 0 );
  clutter_actor_set_position( bottom, 0, -height/2 );
  clutter_actor_set_position( top,    0,  height/2 );

  clutter_actor_show_all( group );
  return group;
}

gint
main( int argc, char *argv[] )
{
  /* utility vars */
  guint boxRow, boxCol;
  GError *error;

  /* dimensions */
  guint stage_width, stage_height, stage_depth, stage_width_far, stage_height_far;
  guint button_width, button_height, button_depth;
  gfloat fovy, aspect, z_near, z_far;

  /* per object properties and objects */
  struct BoxProperties boxProperties[NO_BOXES_X][NO_BOXES_Y];

  /* animation */
  ClutterKnot path_end;

  /* objects */
  ClutterActor *stage;
  ClutterActor *group, *hand;
  GdkPixbuf    *pixbuf;

  /* properties */
  ClutterColor stage_color = { 0xcc, 0xcc, 0xcc, 0xff };

  /* initialise */
  clutter_init( &argc, &argv );
  cogl_enable_depth_test( TRUE );

  /* make hand */
  error = NULL;
  pixbuf = gdk_pixbuf_new_from_file( "redhand.png", &error );
  if (error)
    g_error( "Unable to load redhand.png: %s", error->message );
  hand = clutter_texture_new_from_pixbuf( pixbuf );

  /* set stage */
  stage = clutter_stage_get_default();
  clutter_stage_set_color( CLUTTER_STAGE (stage), &stage_color );
  clutter_stage_set_use_fog( CLUTTER_STAGE (stage), TRUE );
  clutter_stage_set_fog( CLUTTER_STAGE (stage), 1.0, 10, -50 );

  /* initialise dimensions */
  clutter_stage_get_perspective( CLUTTER_STAGE(stage), &fovy, &aspect, &z_near, &z_far );

  clutter_actor_get_size( CLUTTER_ACTOR (stage), &stage_width, &stage_height );
  stage_depth      = z_far;
  stage_width_far  = stage_width*z_far/z_near/100;
  stage_height_far = stage_height*z_far/z_near/100;
  button_width     = stage_width;
  button_height    = stage_height;
  button_depth     = button_width/4;
  path_end.x       = stage_width/2;
  path_end.y       = stage_height/2;

  /* add a group to stage */
  group = clutter_group_new();
  clutter_stage_add( stage, group );
  clutter_actor_show( group );

  /* make new timeline - 3 seconds, at 60 fps */
  for ( boxRow=0; boxRow<NO_BOXES_Y; boxRow++ )
  for ( boxCol=0; boxCol<NO_BOXES_X; boxCol++ )
  {
    /* shorthand */
    struct BoxProperties *thisBoxProperties = &(boxProperties[ boxCol ][ boxRow ]);
    ClutterKnot          *thisPathBegin = &(thisBoxProperties->pathBegin);
    ClutterActor         *thisBox      = thisBoxProperties->box;
    ClutterTimeline      *thisTimeline = thisBoxProperties->timeline;
    ClutterBehaviour     *thisRotY     = thisBoxProperties->rotY;
    ClutterBehaviour     *thisRotZ     = thisBoxProperties->rotZ;
    ClutterBehaviour     *thisTransZ   = thisBoxProperties->transZ;
    ClutterBehaviour     *thisTransXY  = thisBoxProperties->transXY;

    gdouble boxGapX = stage_width/NO_BOXES_X;
    gdouble boxGapY = stage_height/NO_BOXES_Y;

    thisTimeline = clutter_timeline_new (180, 60);

    /* make behaviours */
    /* tumble */
    thisRotY = clutter_behaviour_rotate_new(
      clutter_alpha_new_full( thisTimeline, CLUTTER_ALPHA_SINE, NULL, NULL ),
      CLUTTER_Y_AXIS,
      CLUTTER_ROTATE_CW,
      0, 180 );

    thisRotZ = clutter_behaviour_rotate_new(
      clutter_alpha_new_full( thisTimeline, CLUTTER_ALPHA_SINE_HALF, NULL, NULL ),
      CLUTTER_Z_AXIS,
      CLUTTER_ROTATE_CW,
      0, 90);

    /* translate from back stage to front stage */
    thisTransZ = clutter_behaviour_depth_new(
      clutter_alpha_new_full( thisTimeline, CLUTTER_ALPHA_RAMP_INC, NULL, NULL ),
      -z_far*10,
      -z_near );

    /* calculate the beginning of this button's path to the screen */
    thisPathBegin->x = button_width/2  + ((gint)boxCol*2-(NO_BOXES_X-1))*boxGapX/2*2;
    thisPathBegin->y = button_height/2 + ((gint)boxRow*2-(NO_BOXES_Y-1))*boxGapY/2*2;

    /* create and orient button */
    thisBox = clone_box( CLUTTER_TEXTURE (hand), button_width, button_height, button_depth );
    clutter_container_add_actor( CLUTTER_CONTAINER (stage), thisBox );
    clutter_actor_set_position( thisBox, thisPathBegin->x, thisPathBegin->y );

    /* translate from xy stage to middle of screen */
    thisTransXY = clutter_behaviour_path_new(
      clutter_alpha_new_full( thisTimeline, CLUTTER_ALPHA_RAMP_INC, NULL, NULL ),
      NULL, 0 );
    clutter_behaviour_path_append_knot( CLUTTER_BEHAVIOUR_PATH (thisTransXY), thisPathBegin );
    clutter_behaviour_path_append_knot( CLUTTER_BEHAVIOUR_PATH (thisTransXY), &path_end );

    /* apply behaviours */
    clutter_behaviour_apply( thisRotZ, thisBox );
    clutter_behaviour_apply( thisRotY, thisBox );
    clutter_behaviour_apply( thisTransZ, thisBox );
    clutter_behaviour_apply( thisTransXY, thisBox );

    /* Allow the actor to emit events.  */
    clutter_actor_set_reactive( thisBox, TRUE );
    g_signal_connect( thisBox, "button-release-event",
        G_CALLBACK (on_button_release), thisTimeline  );
  }

  clutter_actor_show( stage );

  clutter_main();

  for ( boxRow=0; boxRow<NO_BOXES_Y; boxRow++ )
  for ( boxCol=0; boxCol<NO_BOXES_X; boxCol++ )
  {
    cleanupBoxProperties( &(boxProperties[ boxCol ][ boxRow ]) );
  }

  return EXIT_SUCCESS;
}
