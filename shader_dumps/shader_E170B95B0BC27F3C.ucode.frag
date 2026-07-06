/*    0.0 */       exec    // PredicateClean=false
/*   13   */          setp_gt r0._, c49.w
/*   14   */          tfetch2D r5.yx_z, r0.xy, tf2
/*   15   */          serialize
                      max r0._, c0, c0
/*    0.1 */ (!p0) jmp L4
/*    1.0 */       exec
/*   16   */          mul r4.xy__, r4.yxxx, c47.yxxx
/*   17   */          tfetch2D r4.xyz_, r4.yx, tf3
/*   18   */          serialize
                      add r4.xyz_, r4.xyzz, -c45.xyzz
/*   19   */          mul_sat r6.xyz_, r_abs[4].xyzz, c46.xyzz
/*   20   */          sgt r7.xyz_, r4.xyzz, -r4.xyzz
/*   21   */          sgt r4.xyz_, -r4.xyzz, r4.xyzz
/*    1.1 */       exec
/*   22   */          add r4.xyz_, r7.xyzz, -r4.xyzz
/*   23   */          mad r4.xyz_, r6.xyzz, r4.xyzz, c253.yyyy
/*   24   */          mul r4.xyz_, r4.xyzz, c49.wwww
/*   25   */          mul_sat r5.xy_w, r4.yxzz, r5.xyww
                label L4
/*    2.0 */       exec
/*   26   */          sgt r4.x___, c47.zzzz, c255.yyyy
              +       snes r0.x___, c49.z
/*   27   */          sge r3.___w, c47.wwww, c255.yyyy
              +       maxs r0._, c49.zz
/*   28   */          sge r7._y__, r5.zzzz, c251.yyyy
              +       adds_prev r0._y__, -c40.x
/*   29   */          mad r4._y__, r0.yyyy, r0.xxxx, c40.xxxx
/*   30   */          dp3 r4.__z_, r3.zxyy, r3.zxyy
/*   31   */          max r5.xy__, r5.xyyy, c251.xxxx
              +       frcs r0.x___, r0.w
/*    2.1 */       exec
/*   32   */          min r0.___w, r5.xxxx, c252.yyyy
              +       adds r0.x___, r0.xx
/*   33   */          sge r0._y__, r0.xxxx, c253.yyyy
              +       rsq r4.__z_, r_abs[4].z
/*   34   */          mul r3.xyz_, r4.zzzz, r3.xyzz
/*   35   */          mul r4.__zw, r0.yyyw, c254.zzzw
              +       maxs r0.___w, c253.yy
/*   36   */          min r5.__z_, r5.yyyy, c252.yyyy
              +       floors r5._y__, r4.w
/*   37   */          dp3_sat r0._y__, r3.zxyy, c36.zxyy
              +       maxs r0._, r0.xx
/*    3.0 */       exec
/*   38   */          sge r7.x___, c253.wwww, r0.yyyy
              +       adds_prev r5.x___, r4.z
/*   39   */          mul r6._yzw, r5.xxyz, c255.zzzz
              +       frcs r7.___w, r0.z
/*   40   */          floor r7.__z_, r6.yyyy
              +       maxs_sat r0.x___, r3.yy
/*   41   */          mul r7, r7, c255
              +       floors r3._y__, r6.w
/*   42   */          add r6.x___, r4.yyyy, r7.zzzz
              +       floors r3.x___, r7.w
/*   43   */          mad r0.__z_, r7.xxxx, r3.wwww, r7.yyyy
/*    3.1 */       exec    // PredicateClean=false
/*   44   */          mad r3.__z_, r5.wwww, c251.zzzz, r0.zzzz
/*   45   */          add r3.xy__, r6.xzzz, r3.xyyy
              +       subsc r3.___w, c253.y, r0.x
/*   46   */          mul r3.xy__, r3.xyyy, c254.xyyy
              +       setp_ne r0._, r4.x
/*    4.0 */ (!p0) jmp L23
/*    4.1 */       exec    // PredicateClean=false
/*   47   */          max r0.__z_, c253.yyyy, c253.yyyy
              +       rcp r0.___w, r1.w
/*   48   */          mul_sat r0.x___, r0.wwww, r1.zzzz
/*   49   */          mul r1.xy__, r0.wwww, r1.xyyy
              +       setp_gt r0._, c31.x
/*   50   */          mad r1.xy__, r1.xyyy, c253.xzzz, c255.yyyy
/*    5.0 */ (!p0) jmp L15
/*    5.1 */       exec
/*   51   */          tfetch2D r0.__y_, r1.xy, tf1
/*   52   */          tfetch2D r0.___z, r1.xy, tf1, OffsetX=1
/*   53   */          tfetch2D r5.z___, r1.xy, tf1, OffsetX=1, OffsetY=-1
/*   54   */          tfetch2D r5._x__, r1.xy, tf1, OffsetX=-1, OffsetY=-1
/*   55   */          tfetch2D r5.__y_, r1.xy, tf1, OffsetY=-1
/*   56   */          tfetch2D r4.x___, r1.xy, tf1, OffsetX=-1
/*    6.0 */       exec
/*   57   */          tfetch2D r4._z__, r1.xy, tf1, OffsetX=1, OffsetY=1
/*   58   */          tfetch2D r4.__x_, r1.xy, tf1, OffsetX=-1, OffsetY=1
/*   59   */          tfetch2D r4.___y, r1.xy, tf1, OffsetY=1
/*   60   */          serialize
                      mul r6.xy__, r1.xyyy, c252.wwww
              +       subsc r0.x___, c253.y, r0.x
/*   61   */          sge r4, r0.xxxx, r4
/*   62   */          sge r1._yzw, r0.xxxx, r5.xxyz
              +       frcs r5.x___, r6.y
/*    6.1 */       exec
/*   63   */          add r5._yzw, r4.yyzw, -r1.yyzw
              +       frcs r1.x___, r6.x
/*   64   */          mad r1._yzw, r5.yyzw, r5.xxxx, r1.yyzw
/*   65   */          sge r0.x_z_, r0.xxxx, r0.zwww
              +       maxs r0._, r1.zz
/*   66   */          add r0.__zw, r1.yyyw, r0.zzzx
              +       adds_prev r0.x___, r4.x
/*   67   */          add r1._y__, r0.zzzz, -r0.xxxx
/*   68   */          mad r0.x___, r1.yyyy, r1.xxxx, r0.xxxx
/*    7.0 */       exec
/*   69   */          add r0.x___, r0.xxxx, r0.wwww
/*   70   */          mulsc r0.__z_, c255.y, r0.x
                label L15
/*    7.1 */       exec
/*   71   */          floor r1.__z_, -r0.yyyy
              +       maxs r0._y__, c253.yy
/*   72   */          add r1._y__, -c30.yyyy, c253.yyyy
              +       rcp r1.x___, r2.w
/*   73   */          mul_sat r0.x___, r1.xxxx, r2.zzzz
              +       floors r0.___w, r3.w
/*   74   */          mad_sat r0.__z_, r1.yyyy, r0.zzzz, c30.yyyy
/*   75   */          mul r1.xy__, r1.xxxx, r2.xyyy
              +       maxs r0._, r0.zz
/*   76   */          mul r1.xy__, r1.xyyy, c253.xzzz
              +       adds_prev_sat r0.__z_, r0.w
/*    8.0 */       exec    // PredicateClean=false
/*   77   */          add r1.xyz_, r1.xyzz, c253.xxyy
              +       setp_gt r0._, c31.x
/*    8.1 */ (!p0) jmp L22
/*    9.0 */       exec
/*   78   */          tfetch2D r0._y__, r1.xy, tf0
/*   79   */          tfetch2D r0.___z, r1.xy, tf0, OffsetX=1
/*   80   */          tfetch2D r4.z___, r1.xy, tf0, OffsetX=1, OffsetY=-1
/*   81   */          tfetch2D r4._x__, r1.xy, tf0, OffsetX=-1, OffsetY=-1
/*   82   */          tfetch2D r4.__y_, r1.xy, tf0, OffsetY=-1
/*   83   */          tfetch2D r2.x___, r1.xy, tf0, OffsetX=-1
/*    9.1 */       exec
/*   84   */          tfetch2D r2._z__, r1.xy, tf0, OffsetX=1, OffsetY=1
/*   85   */          tfetch2D r2.__x_, r1.xy, tf0, OffsetX=-1, OffsetY=1
/*   86   */          tfetch2D r2.___y, r1.xy, tf0, OffsetY=1
/*   87   */          serialize
                      mul r6.xy__, r1.xyyy, c252.wwww
              +       subsc r0.x___, c253.y, r0.x
/*   88   */          sge r2, r0.xxxx, r2
/*   89   */          sge r4.xyz_, r0.xxxx, r4.xyzz
              +       frcs r3.___w, r6.y
/*   10.0 */       exec
/*   90   */          add r5.xyz_, r2.yzww, -r4.xyzz
              +       frcs r1.___w, r6.x
/*   91   */          mad r4.xyz_, r5.xyzz, r3.wwww, r4.xyzz
/*   92   */          sge r0.xy__, r0.xxxx, r0.ywww
              +       maxs r0._, r4.yy
/*   93   */          add r0.xy__, r4.xzzz, r0.yxxx
              +       adds_prev r0.___w, r2.x
/*   94   */          add r2.x___, r0.xxxx, -r0.wwww
/*   95   */          mad r0.___w, r2.xxxx, r1.wwww, r0.wwww
/*   10.1 */       exec
/*   96   */          add r0.x___, r0.wwww, r0.yyyy
/*   97   */          mulsc r0._y__, c255.y, r0.x
                label L22
/*   11.0 */       exec
/*   98   */          add_sat r0.x___, r1.zzzz, r0.yyyy
/*   99   */          min r0.___w, r0.zzzz, r0.xxxx
                label L23
/*   11.1 */       alloc colors
/*   12.0 */       exece
/*  100   */          floors r0.x___, r0.w
/*  101   */          subsc r0.x___, c253.y, r0.x
/*  102   */          mad r3.___w, r0.xxxx, c252.zzzz, c252.xxxx
/*  103   */          max oC0, r3, r3
/*   12.1 */       cnop
