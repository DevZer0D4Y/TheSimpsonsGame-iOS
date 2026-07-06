/*    0.0 */       exec
/*   10   */          tfetch2D r6.xyz_, r0.xy, tf2
/*   11   */          serialize
                      mul r3.___w, c46.xxxx, c253.zzzz
              +       maxs r0._y__, c255.yy
/*   12   */          dp3 r5.x___, r3.zxyy, r3.zxyy
              +       snes r5.___w, c49.z
/*   13   */          sge r6.___w, r4.zzzz, c252.xxxx
              +       maxs r0._, c40.xx
/*   14   */          max r0.x__w, r6.xyyy, c253.yyyy
              +       muls_prev r4.___w, c253.x
/*   15   */          mad r0.__z_, c49.zzzz, c253.xxxx, -r4.wwww
/*    0.1 */       exec
/*   16   */          dp2add r4.x___, r6.zwww, c255.zxxx, c253.yyyy
/*   17   */          min r0.x__w, r0.xwww, c253.wwww
              +       rsq r4._y__, r_abs[5].x
/*   18   */          mul r5.xyz_, r4.yyyy, r3.xyzz
              +       maxs r0._, r0.zz
/*   19   */          dp3_sat r0.__z_, r5.zxyy, c36.zxyy
              +       muls_prev r4._y__, r5.w
/*   20   */          mul r3.xy__, r0.xwww, c254.zzzz
              +       maxs_sat r0.x___, r5.yy
/*   21   */          sge r3.__z_, c254.yyyy, r0.zzzz
              +       floors r0.___w, r3.x
/*    1.0 */       exec    // PredicateClean=false
/*   22   */          mul r4.__z_, r3.wwww, r3.zzzz
              +       floors r3.x___, r3.y
/*   23   */          mad r0.___w, r3.xxxx, c254.zzzz, r0.wwww
/*   24   */          mul r3._y__, r0.wwww, c253.xxxx
              +       subsc r0.___w, c255.y, r0.x
/*   25   */          add r3.x_z_, r4.wzzz, r4.yxxx
              +       setp_ne r0._, c47.x
/*    1.1 */ (!p0) jmp L18
/*    2.0 */       exec    // PredicateClean=false
/*   26   */          max r0._y__, c255.yyyy, c255.yyyy
              +       rcp r1.___w, r1.w
/*   27   */          mul_sat r0.x___, r1.wwww, r1.zzzz
/*   28   */          mul r1.xy__, r1.wwww, r1.xyyy
              +       setp_gt r0._, c31.x
/*   29   */          mad r1.xy__, r1.xyyy, c255.xwww, c255.xxxx
/*    2.1 */ (!p0) jmp L10
/*    3.0 */       exec
/*   30   */          tfetch2D r1.__y_, r1.xy, tf1
/*   31   */          tfetch2D r1.___z, r1.xy, tf1, OffsetX=1
/*   32   */          tfetch2D r5.z___, r1.xy, tf1, OffsetX=1, OffsetY=-1
/*   33   */          tfetch2D r5._x__, r1.xy, tf1, OffsetX=-1, OffsetY=-1
/*   34   */          tfetch2D r5.__y_, r1.xy, tf1, OffsetY=-1
/*   35   */          tfetch2D r4.x___, r1.xy, tf1, OffsetX=-1
/*    3.1 */       exec
/*   36   */          tfetch2D r4._z__, r1.xy, tf1, OffsetX=1, OffsetY=1
/*   37   */          tfetch2D r4.__x_, r1.xy, tf1, OffsetX=-1, OffsetY=1
/*   38   */          tfetch2D r4.___y, r1.xy, tf1, OffsetY=1
/*   39   */          serialize
                      mul r7.xy__, r1.xyyy, c254.wwww
              +       subsc r0.x___, c255.y, r0.x
/*   40   */          sge r4, r0.xxxx, r4
/*   41   */          sge r5.xyz_, r0.xxxx, r5.xyzz
              +       frcs r1.x___, r7.y
/*    4.0 */       exec
/*   42   */          add r6.xyz_, r4.yzww, -r5.xyzz
              +       frcs r0._y__, r7.x
/*   43   */          mad r5.xyz_, r6.xyzz, r1.xxxx, r5.xyzz
/*   44   */          sge r1.xy__, r0.xxxx, r1.zwww
              +       maxs r0._, r5.yy
/*   45   */          add r1.xy__, r5.xzzz, r1.yxxx
              +       adds_prev r0.x___, r4.x
/*   46   */          add r1.__z_, r1.xxxx, -r0.xxxx
/*   47   */          mad r0.x___, r1.zzzz, r0.yyyy, r0.xxxx
/*    4.1 */       exec
/*   48   */          add r0._y__, r0.xxxx, r1.yyyy
/*   49   */          mulsc r0._y__, c255.x, r0.y
                label L10
/*    5.0 */       exec
/*   50   */          floor r1.___w, -r0.zzzz
              +       maxs r1.x___, c255.yy
/*   51   */          add r1._y__, -c30.yyyy, c255.yyyy
              +       rcp r1.__z_, r2.w
/*   52   */          mul_sat r0.x___, r1.zzzz, r2.zzzz
              +       floors r0.__z_, r0.w
/*   53   */          mad_sat r1._y__, r1.yyyy, r0.yyyy, c30.yyyy
/*   54   */          mul r0._y_w, r1.zzzz, r2.xxyy
              +       maxs r0._, r1.yy
/*   55   */          mul r1._yz_, r0.yyww, c255.xxww
              +       adds_prev_sat r0.__z_, r0.z
/*    5.1 */       exec    // PredicateClean=false
/*   56   */          add r1._yzw, r1.yyzw, c255.xxxy
              +       setp_gt r0._, c31.x
/*    6.0 */ (!p0) jmp L17
/*    6.1 */       exec
/*   57   */          tfetch2D r0._y__, r1.yz, tf0
/*   58   */          tfetch2D r0.___z, r1.yz, tf0, OffsetX=1
/*   59   */          tfetch2D r4.z___, r1.yz, tf0, OffsetX=1, OffsetY=-1
/*   60   */          tfetch2D r4._x__, r1.yz, tf0, OffsetX=-1, OffsetY=-1
/*   61   */          tfetch2D r4.__y_, r1.yz, tf0, OffsetY=-1
/*   62   */          tfetch2D r2.x___, r1.yz, tf0, OffsetX=-1
/*    7.0 */       exec
/*   63   */          tfetch2D r2._z__, r1.yz, tf0, OffsetX=1, OffsetY=1
/*   64   */          tfetch2D r2.__x_, r1.yz, tf0, OffsetX=-1, OffsetY=1
/*   65   */          tfetch2D r2.___y, r1.yz, tf0, OffsetY=1
/*   66   */          serialize
                      mul r6.xy__, r1.yzzz, c254.wwww
              +       subsc r0.x___, c255.y, r0.x
/*   67   */          sge r2, r0.xxxx, r2
/*   68   */          sge r4.xyz_, r0.xxxx, r4.xyzz
              +       frcs r3.___w, r6.y
/*    7.1 */       exec
/*   69   */          add r5.xyz_, r2.yzww, -r4.xyzz
              +       frcs r1.x___, r6.x
/*   70   */          mad r4.xyz_, r5.xyzz, r3.wwww, r4.xyzz
/*   71   */          sge r0.xy__, r0.xxxx, r0.ywww
              +       maxs r0._, r4.yy
/*   72   */          add r0.xy__, r4.xzzz, r0.yxxx
              +       adds_prev r0.___w, r2.x
/*   73   */          add r2.x___, r0.xxxx, -r0.wwww
/*   74   */          mad r0.___w, r2.xxxx, r1.xxxx, r0.wwww
/*    8.0 */       exec
/*   75   */          add r0._y__, r0.wwww, r0.yyyy
/*   76   */          mulsc r1.x___, c255.x, r0.y
                label L17
/*    8.1 */       exec
/*   77   */          add_sat r0.x___, r1.wwww, r1.xxxx
/*   78   */          min r0._y__, r0.zzzz, r0.xxxx
                label L18
/*    9.0 */       alloc colors
/*    9.1 */       exece
/*   79   */          floors r0.x___, r0.y
/*   80   */          subsc r0.x___, c255.y, r0.x
/*   81   */          mad r3.___w, r0.xxxx, c252.yyyy, c254.xxxx
/*   82   */          max oC0, r3, r3
