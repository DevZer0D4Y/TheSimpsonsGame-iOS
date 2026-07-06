/*    0.0 */       exec    // PredicateClean=false
/*    3   */          setp_ne r0._, c48.x
/*    4   */          tfetch2D r4.wxyz, r0.xy, tf0
/*    5   */          serialize
                      max r0._, c0, c0
/*    0.1 */ (!p0) jmp L3
/*    1.0 */       exec
/*    6   */          subsc r0.x___, -c255.z, -r4.x
/*    7   */          kill_gt r0._, c255.yyyy, r0.xxxx
                label L3
/*    1.1 */       alloc colors
/*    2.0 */       exec
/*    8   */          add r1.xyz_, -r1.xyzz, c4.xyzz
/*    9   */          dp3 r0.x___, r2.zxyy, r2.zxyy
/*   10   */          dp3 r0.___w, r1.zxyy, r1.zxyy
              +       rsq r0.x___, r_abs[0].x
/*   11   */          mul r0.xyz_, r0.xxxx, r2.xyzz
              +       rsq r0.___w, r_abs[0].w
/*   12   */          mul r1.xyz_, r1.xyzz, r0.wwww
/*   13   */          dp3 r0.x___, r1.zxyy, r0.zxyy
              +       maxs r0._, r4.xx
/*    2.1 */       exece
/*   14   */          max_sat r0.__z_, r_abs[0].xxxx, r_abs[0].xxxx
              +       muls_prev r0._y__, r3.w
/*   15   */          add r0.x___, -r0.zzzz, c255.xxxx
              +       mulsc r0._y__, c40.w, r0.y
/*   16   */          sge r0.__z_, c49.xxxx, r0.zzzz
              +       mulsc_sat r0.x___, c49.y, r0.x
/*   17   */          add r1._yzw, r0.xxxx, -r4.yyzw
              +       subsc r1.x___, c255.x, r0.y
/*   18   */          mad oC0.xyz_, r1.yzww, r0.zzzz, r4.yzww
/*   19   */          mad oC0.___w, r1.xxxx, r0.zzzz, r0.yyyy
