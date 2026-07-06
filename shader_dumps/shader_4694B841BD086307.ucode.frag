/*    0.0 */       exec
/*   10   */          sge r6._y__, r4.zzzz, c253.wwww
/*   11   */          dp3 r0.___w, r3.zxyy, r3.zxyy
              +       frcs r5._y__, r0.y
/*   12   */          mul r0.__z_, c40.xxxx, c253.xxxx
              +       frcs r5.__z_, r0.x
/*   13   */          mad r5.x___, c49.zzzz, c253.xxxx, -r0.zzzz
/*   14   */          add r6.x___, r5.yyyy, r5.yyyy
              +       rsq r0.x___, r_abs[0].w
/*   15   */          mul r4.xyz_, r0.xxxx, r3.xyzz
/*    0.1 */       exec
/*   16   */          sge r6.__z_, r6.xxxx, c254.yyyy
/*   17   */          dp3_sat r0.___w, r4.zxyy, c36.zxyy
              +       snes r3.x___, c49.z
/*   18   */          mul r6._yzw, r6.yyzz, c255.xxyz
              +       maxs r0._, c46.xx
/*   19   */          add r0.xy__, r6.xyyy, r6.zwww
              +       muls_prev r5.___w, c253.y
/*   20   */          sge r3.__z_, c253.yyyy, r0.wwww
              +       mulsc r3._y__, c254.z, r0.x
/*   21   */          mad r3.x_z_, r5.xwww, r3.xzzz, r0.zyyy
/*    1.0 */       exec    // PredicateClean=false
/*   22   */          floors r5.x___, r3.y
/*   23   */          mul r3._y_w, r5.xxzz, c254.zzww
              +       maxs r0._y__, c254.yy
/*   24   */          floor r0.__z_, r3.wwww
              +       maxs_sat r0.x___, r4.yy
/*   25   */          add r3._y__, r3.yyyy, r0.zzzz
              +       subsc r0.__z_, c254.y, r0.x
/*   26   */          mul r3._y__, r3.yyyy, c253.xxxx
              +       setp_ne r0._, c47.x
/*    1.1 */ (!p0) jmp L18
/*    2.0 */       exec    // PredicateClean=false
/*   27   */          max r0._y__, c254.yyyy, c254.yyyy
              +       rcp r1.___w, r1.w
/*   28   */          mul_sat r0.x___, r1.wwww, r1.zzzz
/*   29   */          mul r1.xy__, r1.wwww, r1.xyyy
              +       setp_gt r0._, c31.x
/*   30   */          mad r1.xy__, r1.xyyy, c255.xwww, c255.xxxx
/*    2.1 */ (!p0) jmp L10
/*    3.0 */       exec
/*   31   */          tfetch2D r1.__y_, r1.xy, tf1
/*   32   */          tfetch2D r1.___z, r1.xy, tf1, OffsetX=1
/*   33   */          tfetch2D r5.z___, r1.xy, tf1, OffsetX=1, OffsetY=-1
/*   34   */          tfetch2D r5._x__, r1.xy, tf1, OffsetX=-1, OffsetY=-1
/*   35   */          tfetch2D r5.__y_, r1.xy, tf1, OffsetY=-1
/*   36   */          tfetch2D r4.x___, r1.xy, tf1, OffsetX=-1
/*    3.1 */       exec
/*   37   */          tfetch2D r4._z__, r1.xy, tf1, OffsetX=1, OffsetY=1
/*   38   */          tfetch2D r4.__x_, r1.xy, tf1, OffsetX=-1, OffsetY=1
/*   39   */          tfetch2D r4.___y, r1.xy, tf1, OffsetY=1
/*   40   */          serialize
                      mul r7.xy__, r1.xyyy, c252.yyyy
              +       subsc r0.x___, c254.y, r0.x
/*   41   */          sge r4, r0.xxxx, r4
/*   42   */          sge r5.xyz_, r0.xxxx, r5.xyzz
              +       frcs r1.x___, r7.y
/*    4.0 */       exec
/*   43   */          add r6.xyz_, r4.yzww, -r5.xyzz
              +       frcs r0._y__, r7.x
/*   44   */          mad r5.xyz_, r6.xyzz, r1.xxxx, r5.xyzz
/*   45   */          sge r1.xy__, r0.xxxx, r1.zwww
              +       maxs r0._, r5.yy
/*   46   */          add r1.xy__, r5.xzzz, r1.yxxx
              +       adds_prev r0.x___, r4.x
/*   47   */          add r1.__z_, r1.xxxx, -r0.xxxx
/*   48   */          mad r0.x___, r1.zzzz, r0.yyyy, r0.xxxx
/*    4.1 */       exec
/*   49   */          add r0._y__, r0.xxxx, r1.yyyy
/*   50   */          mulsc r0._y__, c255.x, r0.y
                label L10
/*    5.0 */       exec
/*   51   */          floor r1.__z_, -r0.wwww
              +       maxs r0.___w, c254.yy
/*   52   */          add r1._y__, -c30.yyyy, c254.yyyy
              +       rcp r1.x___, r2.w
/*   53   */          mul_sat r0.x___, r1.xxxx, r2.zzzz
              +       floors r0.__z_, r0.z
/*   54   */          mad_sat r0._y__, r1.yyyy, r0.yyyy, c30.yyyy
/*   55   */          mul r1.xy__, r1.xxxx, r2.xyyy
              +       maxs r0._, r0.yy
/*   56   */          mul r1.xy__, r1.xyyy, c255.xwww
              +       adds_prev_sat r0.__z_, r0.z
/*    5.1 */       exec    // PredicateClean=false
/*   57   */          add r1.xyz_, r1.xyzz, c254.xxyy
              +       setp_gt r0._, c31.x
/*    6.0 */ (!p0) jmp L17
/*    6.1 */       exec
/*   58   */          tfetch2D r0._y__, r1.xy, tf0
/*   59   */          tfetch2D r0.___z, r1.xy, tf0, OffsetX=1
/*   60   */          tfetch2D r4.z___, r1.xy, tf0, OffsetX=1, OffsetY=-1
/*   61   */          tfetch2D r4._x__, r1.xy, tf0, OffsetX=-1, OffsetY=-1
/*   62   */          tfetch2D r4.__y_, r1.xy, tf0, OffsetY=-1
/*   63   */          tfetch2D r2.x___, r1.xy, tf0, OffsetX=-1
/*    7.0 */       exec
/*   64   */          tfetch2D r2._z__, r1.xy, tf0, OffsetX=1, OffsetY=1
/*   65   */          tfetch2D r2.__x_, r1.xy, tf0, OffsetX=-1, OffsetY=1
/*   66   */          tfetch2D r2.___y, r1.xy, tf0, OffsetY=1
/*   67   */          serialize
                      mul r6.xy__, r1.xyyy, c252.yyyy
              +       subsc r0.x___, c254.y, r0.x
/*   68   */          sge r2, r0.xxxx, r2
/*   69   */          sge r4.xyz_, r0.xxxx, r4.xyzz
              +       frcs r3.___w, r6.y
/*    7.1 */       exec
/*   70   */          add r5.xyz_, r2.yzww, -r4.xyzz
              +       frcs r1.___w, r6.x
/*   71   */          mad r4.xyz_, r5.xyzz, r3.wwww, r4.xyzz
/*   72   */          sge r0.xy__, r0.xxxx, r0.ywww
              +       maxs r0._, r4.yy
/*   73   */          add r0.xy__, r4.xzzz, r0.yxxx
              +       adds_prev r0.___w, r2.x
/*   74   */          add r2.x___, r0.xxxx, -r0.wwww
/*   75   */          mad r0.___w, r2.xxxx, r1.wwww, r0.wwww
/*    8.0 */       exec
/*   76   */          add r0._y__, r0.wwww, r0.yyyy
/*   77   */          mulsc r0.___w, c255.x, r0.y
                label L17
/*    8.1 */       exec
/*   78   */          add_sat r0.x___, r1.zzzz, r0.wwww
/*   79   */          min r0._y__, r0.zzzz, r0.xxxx
                label L18
/*    9.0 */       alloc colors
/*    9.1 */       exece
/*   80   */          floors r0.x___, r0.y
/*   81   */          subsc r0.x___, c254.y, r0.x
/*   82   */          mad r3.___w, r0.xxxx, c252.xxxx, c253.zzzz
/*   83   */          max oC0, r3, r3
