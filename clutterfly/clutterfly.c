#include <stdlib.h>
#include <clutter/clutter.h>

const guint noBoxes=4;

static ClutterActor *
clone_box (ClutterTexture *original, guint width, guint height, guint depth)
{
  ClutterActor *group  = clutter_group_new ();

  /* clone texture */
  ClutterActor *front  = clutter_clone_texture_new (original);
  ClutterActor *back   = clutter_clone_texture_new (original);
  ClutterActor *left   = clutter_clone_texture_new (original);
  ClutterActor *right  = clutter_clone_texture_new (original);
  ClutterActor *bottom = clutter_clone_texture_new (original);
  ClutterActor *top    = clutter_clone_texture_new (original);

  /* add each face to the group */
  clutter_container_add_actor (CLUTTER_CONTAINER (group), front);
  clutter_container_add_actor (CLUTTER_CONTAINER (group), back);
  clutter_container_add_actor (CLUTTER_CONTAINER (group), left);
  clutter_container_add_actor (CLUTTER_CONTAINER (group), right);
  clutter_container_add_actor (CLUTTER_CONTAINER (group), bottom);
  clutter_container_add_actor (CLUTTER_CONTAINER (group), top);

  /* move anchor point */
  clutter_actor_set_anchor_point (front,  width/2, height/2);
  clutter_actor_set_anchor_point (back,   width/2, height/2);
  clutter_actor_set_anchor_point (left,   depth/2, height/2);
  clutter_actor_set_anchor_point (right,  depth/2, height/2);
  clutter_actor_set_anchor_point (bottom, width/2, depth/2);
  clutter_actor_set_anchor_point (top,    width/2, depth/2);

  /* set size */
  clutter_actor_set_size (front,  width, height);
  clutter_actor_set_size (back,   width, height);
  clutter_actor_set_size (left,   depth, height);
  clutter_actor_set_size (right,  depth, height);
  clutter_actor_set_size (bottom, width, depth);
  clutter_actor_set_size (top,    width, depth);

  /* orient each face appropriately */
  clutter_actor_set_rotation (front,  CLUTTER_Y_AXIS,    0, 0, 0, 0);
  clutter_actor_set_rotation (back,   CLUTTER_Y_AXIS, -180, 0, 0, 0);
  clutter_actor_set_rotation (left,   CLUTTER_Y_AXIS,   90, 0, 0, 0);
  clutter_actor_set_rotation (right,  CLUTTER_Y_AXIS,  -90, 0, 0, 0);
  clutter_actor_set_rotation (bottom, CLUTTER_X_AXIS,   90, 0, 0, 0);
  clutter_actor_set_rotation (top,    CLUTTER_X_AXIS,  -90, 0, 0, 0);

  /* move them to the correct distance from the origin */
  clutter_actor_set_depth (front, depth/2);
  clutter_actor_set_depth (back, -depth/2);
  clutter_actor_set_position (left,   width/2, 0);
  clutter_actor_set_position (right, -width/2, 0);
  clutter_actor_set_position (bottom, 0, -height/2);
  clutter_actor_set_position (top,    0,  height/2);

  clutter_actor_show_all (group);
  return group;
}

gint
main (int argc, char *argv[])
{
  /* utility vars */
  guint boxIndex;
  GError *error;

  /* dimensions */
  guint stage_width, stage_height, stage_depth, stage_width_far, stage_height_far;
  guint button_width, button_height, button_depth;
  gfloat fovy, aspect, z_near, z_far;

  /* animation */
  ClutterKnot path_begin[noBoxes], path_end;
  ClutterTimeline  *timeline;
  ClutterBehaviour *t_behave_xy[noBoxes];   /* translation from xy to middle of screen */
  ClutterBehaviour *t_behave_z;             /* translation from back stage to front stage */
  ClutterBehaviour *r_behave_z; /* z axis component of tumble */
  ClutterBehaviour *r_behave_y; /* y axis component of tumble */

  /* objects */
  ClutterActor     *stage;
  ClutterActor     *group, *hand, *label, *rect, *janus, *box[noBoxes];
  GdkPixbuf        *pixbuf;

  /* properties */
  ClutterColor      stage_color = { 0xcc, 0xcc, 0xcc, 0xff };
  ClutterColor      rect_color  = { 0, 0, 0, 0x88 };

  /* initialise */
  clutter_init (&argc, &argv);
  cogl_enable_depth_test( TRUE );

  /* make hand */
  error = NULL;
  pixbuf = gdk_pixbuf_new_from_file ("redhand.png", &error);
  if (error)
    g_error ("Unable to load redhand.png: %s", error->message);
  hand = clutter_texture_new_from_pixbuf (pixbuf);

  /* set stage */
  stage = clutter_stage_get_default ();
  clutter_stage_set_color (CLUTTER_STAGE (stage), &stage_color);
  clutter_stage_set_use_fog (CLUTTER_STAGE (stage), TRUE);
  clutter_stage_set_fog (CLUTTER_STAGE (stage), 1.0, 10, -50);

  /* initialise dimensions */
  clutter_stage_get_perspective (CLUTTER_STAGE(stage), &fovy, &aspect, &z_near, &z_far);
  clutter_actor_get_size (CLUTTER_ACTOR (stage), &stage_width, &stage_height);
  stage_depth      = stage_width*4;
  stage_width_far  = stage_width*z_far/z_near;
  stage_height_far = stage_height*z_far/z_near;
  button_width     = stage_width;
  button_height    = stage_height;
  button_depth     = button_width/4;
  path_end.x       = stage_width/2;
  path_end.y       = stage_height/2;


  /* signal to quit */
  g_signal_connect (stage,
                    "button-press-event", G_CALLBACK (clutter_main_quit),
                    NULL);

  /* add a group to stage */
  group = clutter_group_new ();
  clutter_stage_add (stage, group);
  clutter_actor_show (group);

  /* make new timeline - 3 seconds, at 60 fps */
  timeline = clutter_timeline_new (180, 60);

  /* make behaviours */
  /* tumble */
  r_behave_y = clutter_behaviour_rotate_new (
    clutter_alpha_new_full (
      timeline,
      CLUTTER_ALPHA_RAMP_INC,
      NULL, NULL),
    CLUTTER_Y_AXIS,
    CLUTTER_ROTATE_CW,
    0, 180);

  r_behave_z = clutter_behaviour_rotate_new (
    clutter_alpha_new_full (
      timeline,
      CLUTTER_ALPHA_SINE_HALF,
      NULL, NULL),
    CLUTTER_Z_AXIS,
    CLUTTER_ROTATE_CW,
    0, 90);

  /* translate from back stage to front stage */
  t_behave_z = clutter_behaviour_depth_new (
    clutter_alpha_new_full (
      timeline,
      CLUTTER_ALPHA_RAMP_INC,
      NULL, NULL),
    -stage_depth,
    0);

  for ( boxIndex=0; boxIndex<noBoxes; boxIndex++ )
  {
    path_begin[boxIndex].x = (boxIndex%2)*stage_width_far;
    path_begin[boxIndex].y = (boxIndex/2)*stage_height_far;

    /* create and orient button */
    box[boxIndex] = clone_box (CLUTTER_TEXTURE (hand), button_width, button_height, button_depth);
    clutter_container_add_actor (CLUTTER_CONTAINER (stage), box[boxIndex]);
    clutter_actor_set_position (box[boxIndex], path_begin[boxIndex].x, path_begin[boxIndex].y);
    clutter_actor_set_size (box[boxIndex], button_width, button_height);

    /* translate from xy stage to middle of screen */
    t_behave_xy[ boxIndex ] = clutter_behaviour_path_new (
      clutter_alpha_new_full ( timeline, CLUTTER_ALPHA_RAMP_INC, NULL, NULL),
      NULL, 0 );
    clutter_behaviour_path_append_knot ( CLUTTER_BEHAVIOUR_PATH (t_behave_xy[ boxIndex ]), &path_begin[ boxIndex ] );
    clutter_behaviour_path_append_knot ( CLUTTER_BEHAVIOUR_PATH (t_behave_xy[ boxIndex ]), &path_end );

    /* apply behaviours */
    clutter_behaviour_apply (r_behave_z, box[boxIndex]);
    clutter_behaviour_apply (r_behave_y, box[boxIndex]);
    clutter_behaviour_apply (t_behave_z, box[boxIndex]);
    clutter_behaviour_apply (t_behave_xy[boxIndex], box[boxIndex]);
  }

  clutter_actor_show (stage);

  clutter_timeline_start (timeline);

  clutter_main ();

  g_object_unref (r_behave_z);
  g_object_unref (r_behave_y);
  g_object_unref (t_behave_z);
  for ( boxIndex=0; boxIndex<noBoxes; boxIndex++ )
	  g_object_unref (t_behave_xy[ boxIndex ]);
  g_object_unref (timeline);

  return EXIT_SUCCESS;
}
