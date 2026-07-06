/*    0.0 */       exec
/*    2   */          tfetch2D r1.x___, r0.xy, tf0
/*    3   */          tfetch2D r1._x__, r0.xy, tf1
/*    4   */          tfetch2D r1.__x_, r0.xy, tf2
/*    0.1 */       alloc colors
/*    1.0 */       exece
/*    5   */          add r0.xyz_, r1.xyzz, c253.zwww
/*    6   */          dp2add oC0.x000, r0.xyyy, c253.xyyy, c255.xxxx
/*    7   */          dp3 oC0._y__, r0.yzxx, c254.xyzz
/*    8   */          dp2add oC0.__z_, r0.xzzz, c254.zwww, c255.xxxx
/*    1.1 */       cnop
