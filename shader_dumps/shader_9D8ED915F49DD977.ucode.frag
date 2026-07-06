/*    0.0 */       exec
/*    2   */          tfetch2D r0, r0.xy, tf0
/*    0.1 */       alloc colors
/*    1.0 */       exece
/*    3   */          dp2add oC0.x___, r0.xxxx, r2.xxxx, c255.xxxx
/*    4   */          dp2add oC0._y__, r0.yyyy, r2.yyyy, c255.xxxx
/*    5   */          dp2add oC0.__z_, r0.zzzz, r2.zzzz, c255.xxxx
/*    6   */          dp2add oC0.___w, r0.wwww, r2.wwww, c255.xxxx
/*    1.1 */       cnop
