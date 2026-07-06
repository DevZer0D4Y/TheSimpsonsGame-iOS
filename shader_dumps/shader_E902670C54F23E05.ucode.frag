/*    0.0 */       exec
/*   11   */          tfetch2D r6.xyz_, r0.xy, tf2
/*   12   */          serialize
                      mul r1.__z_, c46.xxxx, c251.zzzz
/*   13   */          add r1.___w, c49.zzzz, -c40.xxxx
/*   14   */          sge r6.___w, r5.zzzz, c255.xxxx
/*   15   */          dp3 r0.x___, r4.zxyy, r4.zxyy
              +       frcs r7.__z_, r1.y
/*   16   */          max r0._yz_, r6.xxyy, c251.yyyy
              +       frcs r7.___w, r1.x
/*    0.1 */       exec
/*   17   */          min r7.xy__, r0.yzzz, c251.wwww
              +       rsq r0.x___, r_abs[0].x
/*   18   */          mul r5.xyz_, r0.xxxx, r4.xyzz
              +       adds r0.x___, r7.zz
/*   19   */          sge r0._y__, r0.xxxx, c253.yyyy
/*   20   */          dp3_sat r0.__z_, r5.zxyy, c36.zxyy
/*   21   */          mad r0.x___, r0.yyyy, c252.zzzz, r0.xxxx
/*   22   */          mul r4.xyz_, r7.wxyy, c254.wzzz
/*    1.0 */       exec
/*   23   */          sge r0._y__, c252.yyyy, r0.zzzz
              +       mulsc r4.___w, c254.z, r0.x
/*   24   */          floor r4, r4
              +       snes r0.___w, c49.z
/*   25   */          mul r0.xy__, r1.zwww, r0.ywww
              +       maxs r0.___w, c253.yy
/*   26   */          dp2add r1.___w, r6.zwww, c253.wxxx, r0.xxxx
/*   27   */          mul r1._yz_, r4.wwzz, c254.zzzz
              +       addsc r0.x___, c40.x, r0.y
/*   28   */          add r1.x___, r0.xxxx, r1.yyyy
              +       maxs_sat r0.x___, r5.yy
/*    1.1 */       exec    // PredicateClean=false
/*   29   */          add r1._yz_, r1.xxzz, r4.xxyy
              +       subsc r1.x___, c253.y, r0.x
/*   30   */          mul r1._yz_, r1.yyzz, c254.xxyy
              +       setp_ne r0._, c47.x
/*    2.0 */ (!p0) jmp L19
/*    2.1 */       exec    // PredicateClean=false
/*   31   */          max r0._y__, c253.yyyy, c253.yyyy
              +       rcp r0.___w, r2.w
/*   32   */          mul_sat r0.x___, r0.wwww, r2.zzzz
/*   33   */          mul r2.xy__, r0.wwww, r2.xyyy
              +       setp_gt r0._, c31.x
/*   34   */          mad r2.xy__, r2.xyyy, c253.xzzz, c253.xxxx
/*    3.0 */ (!p0) jmp L11
/*    3.1 */       exec
/*   35   */          tfetch2D r0._y__, r2.xy, tf1
/*   36   */          tfetch2D r0.___z, r2.xy, tf1, OffsetX=1
/*   37   */          tfetch2D r5.z___, r2.xy, tf1, OffsetX=1, OffsetY=-1
/*   38   */          tfetch2D r5._x__, r2.xy, tf1, OffsetX=-1, OffsetY=-1
/*   39   */          tfetch2D r5.__y_, r2.xy, tf1, OffsetY=-1
/*   40   */          tfetch2D r4.x___, r2.xy, tf1, OffsetX=-1
/*    4.0 */       exec
/*   41   */          tfetch2D r4._z__, r2.xy, tf1, OffsetX=1, OffsetY=1
/*   42   */          tfetch2D r4.__x_, r2.xy, tf1, OffsetX=-1, OffsetY=1
/*   43   */          tfetch2D r4.___y, r2.xy, tf1, OffsetY=1
/*   44   */          serialize
                      mul r6.xy__, r2.xyyy, c252.wwww
              +       subsc r0.x___, c253.y, r0.x
/*   45   */          sge r4, r0.xxxx, r4
/*   46   */          sge r2._yzw, r0.xxxx, r5.xxyz
              +       frcs r5.x___, r6.y
/*    4.1 */       exec
/*   47   */          add r5._yzw, r4.yyzw, -r2.yyzw
              +       frcs r2.x___, r6.x
/*   48   */          mad r2._yzw, r5.yyzw, r5.xxxx, r2.yyzw
/*   49   */          sge r0.xy__, r0.xxxx, r0.ywww
              +       maxs r0._, r2.zz
/*   50   */          add r0._y_w, r2.yyww, r0.yyxx
              +       adds_prev r0.x___, r4.x
/*   51   */          add r2._y__, r0.yyyy, -r0.xxxx
/*   52   */          mad r0.x___, r2.yyyy, r2.xxxx, r0.xxxx
/*    5.0 */       exec
/*   53   */          add r0._y__, r0.xxxx, r0.wwww
/*   54   */          mulsc r0._y__, c253.x, r0.y
                label L11
/*    5.1 */       exec
/*   55   */          floor r2.__z_, -r0.zzzz
              +       maxs r0.___w, c253.yy
/*   56   */          add r2._y__, -c30.yyyy, c253.yyyy
              +       rcp r2.x___, r3.w
/*   57   */          mul_sat r0.x___, r2.xxxx, r3.zzzz
              +       floors r0.__z_, r1.x
/*   58   */          mad_sat r0._y__, r2.yyyy, r0.yyyy, c30.yyyy
/*   59   */          mul r2.xy__, r2.xxxx, r3.xyyy
              +       maxs r0._, r0.yy
/*   60   */          mul r2.xy__, r2.xyyy, c253.xzzz
              +       adds_prev_sat r0.__z_, r0.z
/*    6.0 */       exec    // PredicateClean=false
/*   61   */          add r2.xyz_, r2.xyzz, c253.xxyy
              +       setp_gt r0._, c31.x
/*    6.1 */ (!p0) jmp L18
/*    7.0 */       exec
/*   62   */          tfetch2D r0._y__, r2.xy, tf0
/*   63   */          tfetch2D r0.___z, r2.xy, tf0, OffsetX=1
/*   64   */          tfetch2D r4.z___, r2.xy, tf0, OffsetX=1, OffsetY=-1
/*   65   */          tfetch2D r4._x__, r2.xy, tf0, OffsetX=-1, OffsetY=-1
/*   66   */          tfetch2D r4.__y_, r2.xy, tf0, OffsetY=-1
/*   67   */          tfetch2D r3.x___, r2.xy, tf0, OffsetX=-1
/*    7.1 */       exec
/*   68   */          tfetch2D r3._z__, r2.xy, tf0, OffsetX=1, OffsetY=1
/*   69   */          tfetch2D r3.__x_, r2.xy, tf0, OffsetX=-1, OffsetY=1
/*   70   */          tfetch2D r3.___y, r2.xy, tf0, OffsetY=1
/*   71   */          serialize
                      mul r6.xy__, r2.xyyy, c252.wwww
              +       subsc r0.x___, c253.y, r0.x
/*   72   */          sge r3, r0.xxxx, r3
/*   73   */          sge r4.xyz_, r0.xxxx, r4.xyzz
              +       frcs r2.___w, r6.y
/*    8.0 */       exec
/*   74   */          add r5.xyz_, r3.yzww, -r4.xyzz
              +       frcs r1.x___, r6.x
/*   75   */          mad r4.xyz_, r5.xyzz, r2.wwww, r4.xyzz
/*   76   */          sge r0.xy__, r0.xxxx, r0.ywww
              +       maxs r0._, r4.yy
/*   77   */          add r0.xy__, r4.xzzz, r0.yxxx
              +       adds_prev r0.___w, r3.x
/*   78   */          add r2.___w, r0.xxxx, -r0.wwww
/*   79   */          mad r0.___w, r2.wwww, r1.xxxx, r0.wwww
/*    8.1 */       exec
/*   80   */          add r0._y__, r0.wwww, r0.yyyy
/*   81   */          mulsc r0.___w, c253.x, r0.y
                label L18
/*    9.0 */       exec
/*   82   */          add_sat r0.x___, r2.zzzz, r0.wwww
/*   83   */          min r0.___w, r0.zzzz, r0.xxxx
                label L19
/*    9.1 */       alloc colors
/*   10.0 */       exece
/*   84   */          floors r0.x___, r0.w
/*   85   */          subsc r0.x___, c253.y, r0.x
/*   86   */          mad r1.x___, r0.xxxx, c252.xxxx, c251.xxxx
/*   87   */          max oC0, r1.yzwx, r1.yzwx
/*   10.1 */       cnop
