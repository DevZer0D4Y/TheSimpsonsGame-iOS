/*    0.0 */       exec
/*    6   */          sgts r2.__z_, -r_abs[0].x
/*    7   */          max r1._yz_, c255.xxyy, c255.xxyy
              +       subs r2._y__, r0.yw
/*    8   */          mul r0._y_w, r0.wwzz, c255.zzzz
              +       subs r0.x___, r0.xz
/*    0.1 */       loop i31, L9
                label L2
/*    1.0 */       exec
/*    9   */          max r1.___w, c255.xxxx, c255.xxxx
              +       maxs r2.x___, r0.xx
/*    1.1 */       loop i30, L7
                label L4
/*    2.0 */       exec
/*   10   */          tfetch2D r2.___x, r2.xy, tf0, FetchValidOnly=false
/*   11   */          serialize
                      dp2add r0.__z_, r1.wyyy, r1.wyyy, c255.yyyy
/*   12   */          add r1.___w, r1.wwww, c255.zzzz
              +       sqrt r0.__z_, r_abs[0].z
/*   13   */          min r3._y__, r0.zzzz, c255.wwww
/*   14   */          add r0.__z_, -r3.yyyy, c255.wwww
              +       addsc r1.__z_, c255.w, r1.z
/*   15   */          sge r3.x___, r2.wwww, r1.xxxx
              +       maxs r0._, r1.zz
/*    2.1 */       exec
/*   16   */          add r2.x__w, r0.wzzz, r2.xzzz
              +       adds_prev r1.__z_, -r3.y
/*   17   */          cndeq r2.__z_, r3.xxxx, r2.zzzz, r2.wwww
/*    3.0 */       endloop i30, L4
                label L7
/*    3.1 */       exec
/*   18   */          add r2._y__, r0.yyyy, r2.yyyy
              +       addsc r1._y__, c255.z, r1.y
/*    4.0 */       endloop i31, L2
                label L9
/*    4.1 */       alloc colors
/*    5.0 */       exece
/*   19   */          rcp r0.x___, r1.z
/*   20   */          mad oC0.xyz1, -r0.xxxx, r2.zzzz, c255.wwww
/*    5.1 */       cnop
