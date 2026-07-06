/*    0.0 */       alloc colors
/*    0.1 */       exec
/*    4   */          add r7.xyz_, -r2.xyzz, c4.xyzz
              +       snes r4.__z_, c49.z
/*    5   */          sge r3.___w, c39.wwww, c253.wwww
              +       floors r3.x___, r3.x
/*    6   */          dp3 r0.__z_, c32.zxyy, c32.zxyy
              +       maxs r0._, c40.yy
/*    7   */          sge r1.___w, r3.zzzz, c254.wwww
              +       adds_prev r2.___w, c253.y
/*    8   */          dp3 r0.___w, r1.zxyy, r1.zxyy
              +       frcs r5.x___, r0.y
/*    9   */          mul r2.__z_, c40.xxxx, c252.xxxx
              +       frcs r5._y__, r0.x
/*    1.0 */       exec
/*   10   */          maxs oC0.___w, c255.ww
/*   11   */          mad r5.__z_, c49.zzzz, c252.xxxx, -r2.zzzz
/*   12   */          mul r3.__z_, r1.wwww, c253.wwww
              +       rsq r0._y__, r_abs[0].w
/*   13   */          add r3._y__, r5.xxxx, r5.xxxx
              +       rsq r0.___w, r_abs[0].z
/*   14   */          dp3 r0.x___, r7.zxyy, r7.zxyy
              +       mulsc r6.x___, c32.x, r0.w
/*   15   */          mul r4.xy_w, r0.yyyy, r1.xyzz
              +       mulsc r6._y__, c32.y, r0.w
/*    1.1 */       exec
/*   16   */          sge r5.___w, r3.yyyy, c253.yyyy
              +       rsq r0.x___, r_abs[0].x
/*   17   */          mul r0.xyz_, r7.yxzz, r0.xxxx
              +       mulsc r6.__z_, c32.z, r0.w
/*   18   */          dp3 r0.___w, r6.zxyy, r4.wxyy
              +       maxs r0._, c33.yy
/*   19   */          mul r6.x_zw, r5.ywww, c254.xyyz
              +       adds_prev r1.x___, c253.y
/*   20   */          add r2.xy__, r3.yzzz, r6.zwww
              +       addsc r1._y__, c40.z, r0.y
/*   21   */          cndeq r1._yzw, c253.xxyy, r1.yyyy, r0.zzzx
/*    2.0 */       exec
/*   22   */          dp3_sat r1._y__, r1.zwyy, r4.wyxx
              +       sges r1.___w, -r0.w
/*   23   */          mul r6._y__, r2.xxxx, c252.yyyy
              +       addsc r0.___w, c33.x, r0.y
/*   24   */          dp3_sat r1.__z_, r0.zxww, r4.wyxx
              +       floors r3._y__, r6.x
/*   25   */          mul r0, r1.wzwy, c255.zxyx
              +       floors r3.__z_, r6.y
/*   26   */          add r1.__z_, r0.yyyy, c33.yyyy
              +       rcp r1.x___, r1.x
/*   27   */          add r1.___w, r0.wwww, c40.yyyy
              +       rcp r1._y__, r2.w
/*    2.1 */       exec
/*   28   */          mad r0._y__, r3.zzzz, c252.yyyy, r3.yyyy
/*   29   */          mul r1._yz_, r1.zzww, r1.xxyy
              +       subsc r1.x___, c253.y, r3.x
/*   30   */          mul r1._yz_, r1.yyzz, r1.yyzz
              +       maxs r0.x___, r0.xx
/*   31   */          sge r4.x___, c33.zzzz, r1.yyyy
              +       addsc r5.x___, c253.z, r0.x
/*   32   */          sge r4._y__, c255.yyyy, r1.zzzz
              +       addsc r5._y__, c253.x, r0.z
/*   33   */          mul r1._yzw, r5.xxyz, r4.xxyz
              +       subsc r0.x___, c253.y, r3.w
/*    3.0 */       exece
/*   34   */          dp2add r0.__z_, r1.zyyy, r0.xxxx, c253.xxxx
/*   35   */          mulsc r0.x___, c47.x, r0.z
/*   36   */          mul r1.x___, r0.xxxx, r1.xxxx
/*   37   */          add oC0.x_z_, r2.zyyy, r1.wxxx
              +       mulsc oC0._y__, c252.x, r0.y
/*    3.1 */       cnop
