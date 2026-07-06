/*    0.0 */       exec
/*    2   */          add r0.__z_, c23.zzzz, -c23.xxxx
              +       rcp r0.___w, r3.w
/*    3   */          mul r1.__zw, r0.wwww, r3.yyyx
/*    4   */          mad r3._yz_, r1.zzww, c254.yyxx, c254.xxxx
/*    5   */          mad r3.x___, r3.zzzz, r0.zzzz, c23.xxxx
/*    6   */          tfetch2D r0.__x_, r3.xy, tf3
/*    7   */          serialize
                      subsc r0.__z_, c255.x, r0.z
/*    0.1 */       exec
/*    8   */          kill_gt r0._, c254.zzzz, r0.zzzz
/*    9   */          tfetch2D r0._xyz, r0.xy, tf0
/*   10   */          tfetch2D r3, r1.xy, tf1
/*   11   */          tfetch2D r1, r2.xy, tf2
/*    1.0 */       alloc colors
/*    1.1 */       exece
/*   12   */          add r2.x___, -r3.wwww, c254.wwww
              +       maxs r0.x___, r1.ww
/*   13   */          mul r2.xyz_, r2.xxxx, r0.yzww
              +       subsc r0.___w, c254.w, r0.x
/*   14   */          mad r0.xyz_, r3.wwww, r3.xyzz, r2.xyzz
/*   15   */          mul r0.xyz_, r0.wwww, r0.xyzz
/*   16   */          mad oC0.xyz1, r1.wwww, r1.xyzz, r0.xyzz
