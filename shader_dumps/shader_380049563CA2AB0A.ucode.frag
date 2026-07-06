/*    0.0 */       exec
/*    3   */          tfetch2D r1.zyx_, r0.xy, tf0
/*    4   */          serialize
                      max r0._, c0, c0
/*    0.1 */       loop i31, L4
                label L2
/*    1.0 */       exec
/*    5   */          rcp r0.__z_, c50.x
/*    6   */          rcp r0.___w, c49.x
/*    7   */          mad r0.__zw, r0.wwwz, c[20+aL].yyyx, r0.yyyx
/*    8   */          tfetch2D r2.xyz_, r0.wz, tf0
/*    9   */          serialize
                      add r1.xyz_, r2.zyxx, r1.xyzz
/*    1.1 */       endloop i31, L2
                label L4
/*    2.0 */       alloc colors
/*    2.1 */       exece
/*   10   */          mul oC0.xyz1, r1.zyxx, c255.xxxx
