/*    0.0 */       exec
/*   12   */          tfetch2D r8, r0.xy, tf2
/*   13   */          serialize
                      add r10.___w, c49.zzzz, -c40.xxxx
/*   14   */          dp3 r4.___w, r4.zxyy, r4.zxyy
/*   15   */          add r10.xyz_, -r1.xyzz, c4.xyzz
              +       frcs r6.__z_, r0.w
/*   16   */          dp3 r1.x___, r10.zxyy, r10.zxyy
              +       frcs r6.___w, r0.z
/*   17   */          add r5._y__, r6.zzzz, r6.zzzz
              +       snes r0.__z_, c49.z
/*    0.1 */       exec
/*   18   */          sge r0.x___, r5.yyyy, c253.yyyy
              +       rsq r0._y__, r_abs[4].w
/*   19   */          mul r9.xyz_, r0.yyyy, r4.xyzz
              +       rsq r0._y__, r_abs[1].x
/*   20   */          mul r11, r10, r0.yyyz
/*   21   */          dp2add r0._y__, r11.zyyy, r11.zyyy, c255.xxxx
/*   22   */          add r1.xy__, r11.wxxx, c40.xzzz
              +       mulsc r0.__z_, c252.z, r0.x
/*   23   */          dp3_sat r1.__z_, r9.zxyy, c36.zxyy
              +       muls r5.x___, r1.yy
/*    1.0 */       exec
/*   24   */          add r5.xy__, r5.xyyy, r0.yzzz
              +       maxs r12.___w, c253.yy
/*   25   */          sge r4.x___, c252.yyyy, r1.zzzz
              +       rsq r0._y__, r_abs[5].x
/*   26   */          dp3 r0.x___, r11.zxyy, r11.zxyy
              +       maxs r0._, r1.yy
/*   27   */          mul r12._yz_, r11.yyzz, r0.yyyy
              +       muls_prev r12.x___, r0.y
/*   28   */          dp3 r9.___w, r9.zxyy, r12.zxyy
              +       maxs r0._, c45.xx
/*   29   */          dp4 r0._y__, r9.zxyw, r12.zxyw
              +       muls_prev r10.x___, c251.z
/*    1.1 */       exec
/*   30   */          mad r10._yzw, r9.yyzx, r0.yyyy, -r12.yyzx
/*   31   */          dp3 r0.___w, r10.zwyy, r10.zwyy
              +       rsq r0.x___, r_abs[0].x
/*   32   */          mul r0.xyz_, r11.xyzz, r0.xxxx
              +       rsq r4._y__, r_abs[0].w
/*   33   */          mul r4, r10.xwyz, r4.xyyy
/*   34   */          dp3 r0.x___, r4.wyzz, r0.zxyy
/*   35   */          max r0.x___, r0.xxxx, c255.xxxx
              +       maxs_sat r0._y__, r9.yy
/*    2.0 */       exec
/*   36   */          add r0.__z_, -r0.yyyy, c253.yyyy
              +       log r0._y__, r_abs[0].x
/*   37   */          mulsc r0.x___, c50.x, r0.y
/*   38   */          mul r5.___w, r5.yyyy, c254.zzzz
              +       exp r0._y__, r0.x
/*   39   */          sge r7.___w, r5.zzzz, c251.yyyy
              +       mulsc r0.x___, c47.x, r0.y
/*   40   */          mad_sat r7.xyz_, r0.xxxx, r8.wwww, r8.xzyy
/*   41   */          min r6.xy__, r7.xzzz, c251.wwww
/*    2.1 */       exec
/*   42   */          dp2add r4.__z_, r7.ywww, c253.wxxx, r4.xxxx
/*   43   */          mul r5.xyz_, r6.wxyy, c254.wzzz
/*   44   */          floor r5, r5
/*   45   */          mul r0._y_w, r5.wwzz, c254.zzzz
/*   46   */          add r0.x___, r1.xxxx, r0.yyyy
/*   47   */          add r0._y_w, r0.xxww, r5.xxyy
              +       maxs r0.x___, c253.yy
/*    3.0 */       exec    // PredicateClean=false
/*   48   */          mul r4.xy__, r0.ywww, c254.xyyy
              +       setp_ne r0._, c46.x
/*    3.1 */ (!p0) jmp L22
/*    4.0 */       exec    // PredicateClean=false
/*   49   */          max r0.___w, c253.yyyy, c253.yyyy
              +       rcp r0._y__, r2.w
/*   50   */          mul_sat r0.x___, r0.yyyy, r2.zzzz
/*   51   */          mul r1.xy__, r0.yyyy, r2.xyyy
              +       setp_gt r0._, c31.x
/*   52   */          mad r1.xy__, r1.xyyy, c253.xzzz, c253.xxxx
/*    4.1 */ (!p0) jmp L14
/*    5.0 */       exec
/*   53   */          tfetch2D r0._y__, r1.xy, tf1
/*   54   */          tfetch2D r0.___z, r1.xy, tf1, OffsetX=1
/*   55   */          tfetch2D r5.z___, r1.xy, tf1, OffsetX=1, OffsetY=-1
/*   56   */          tfetch2D r5._x__, r1.xy, tf1, OffsetX=-1, OffsetY=-1
/*   57   */          tfetch2D r5.__y_, r1.xy, tf1, OffsetY=-1
/*   58   */          tfetch2D r2.x___, r1.xy, tf1, OffsetX=-1
/*    5.1 */       exec
/*   59   */          tfetch2D r2._z__, r1.xy, tf1, OffsetX=1, OffsetY=1
/*   60   */          tfetch2D r2.__x_, r1.xy, tf1, OffsetX=-1, OffsetY=1
/*   61   */          tfetch2D r2.___y, r1.xy, tf1, OffsetY=1
/*   62   */          serialize
                      mul r1.x__w, r1.xyyy, c252.wwww
              +       subsc r0.x___, c253.y, r0.x
/*   63   */          sge r2, r0.xxxx, r2
/*   64   */          sge r5.xyz_, r0.xxxx, r5.xyzz
              +       frcs r1._y__, r1.w
/*    6.0 */       exec
/*   65   */          add r6.xyz_, r2.yzww, -r5.xyzz
              +       frcs r1.x___, r1.x
/*   66   */          mad r5.xyz_, r6.xyzz, r1.yyyy, r5.xyzz
/*   67   */          sge r0.xy__, r0.xxxx, r0.ywww
              +       maxs r0._, r5.yy
/*   68   */          add r0._y_w, r5.xxzz, r0.yyxx
              +       adds_prev r0.x___, r2.x
/*   69   */          add r1._y__, r0.yyyy, -r0.xxxx
/*   70   */          mad r0.x___, r1.yyyy, r1.xxxx, r0.xxxx
/*    6.1 */       exec
/*   71   */          add r0._y__, r0.xxxx, r0.wwww
/*   72   */          mulsc r0.___w, c253.x, r0.y
                label L14
/*    7.0 */       exec
/*   73   */          floor r1.__z_, -r1.zzzz
              +       maxs r0._y__, c253.yy
/*   74   */          add r1._y__, -c30.yyyy, c253.yyyy
              +       rcp r1.x___, r3.w
/*   75   */          mul_sat r0.x___, r1.xxxx, r3.zzzz
              +       floors r0.__z_, r0.z
/*   76   */          mad_sat r0.___w, r1.yyyy, r0.wwww, c30.yyyy
/*   77   */          mul r1.xy__, r1.xxxx, r3.xyyy
              +       maxs r0._, r0.ww
/*   78   */          mul r1.xy__, r1.xyyy, c253.xzzz
              +       adds_prev_sat r0.__z_, r0.z
/*    7.1 */       exec    // PredicateClean=false
/*   79   */          add r1.xyz_, r1.xyzz, c253.xxyy
              +       setp_gt r0._, c31.x
/*    8.0 */ (!p0) jmp L21
/*    8.1 */       exec
/*   80   */          tfetch2D r0._y__, r1.xy, tf0
/*   81   */          tfetch2D r0.___z, r1.xy, tf0, OffsetX=1
/*   82   */          tfetch2D r3.z___, r1.xy, tf0, OffsetX=1, OffsetY=-1
/*   83   */          tfetch2D r3._x__, r1.xy, tf0, OffsetX=-1, OffsetY=-1
/*   84   */          tfetch2D r3.__y_, r1.xy, tf0, OffsetY=-1
/*   85   */          tfetch2D r2.x___, r1.xy, tf0, OffsetX=-1
/*    9.0 */       exec
/*   86   */          tfetch2D r2._z__, r1.xy, tf0, OffsetX=1, OffsetY=1
/*   87   */          tfetch2D r2.__x_, r1.xy, tf0, OffsetX=-1, OffsetY=1
/*   88   */          tfetch2D r2.___y, r1.xy, tf0, OffsetY=1
/*   89   */          serialize
                      mul r6.xy__, r1.xyyy, c252.wwww
              +       subsc r0.x___, c253.y, r0.x
/*   90   */          sge r2, r0.xxxx, r2
/*   91   */          sge r3.xyz_, r0.xxxx, r3.xyzz
              +       frcs r3.___w, r6.y
/*    9.1 */       exec
/*   92   */          add r5.xyz_, r2.yzww, -r3.xyzz
              +       frcs r1.___w, r6.x
/*   93   */          mad r3.xyz_, r5.xyzz, r3.wwww, r3.xyzz
/*   94   */          sge r0.xy__, r0.xxxx, r0.ywww
              +       maxs r0._, r3.yy
/*   95   */          add r0.xy__, r3.xzzz, r0.yxxx
              +       adds_prev r0.___w, r2.x
/*   96   */          add r2.x___, r0.xxxx, -r0.wwww
/*   97   */          mad r0.___w, r2.xxxx, r1.wwww, r0.wwww
/*   10.0 */       exec
/*   98   */          add r0._y__, r0.wwww, r0.yyyy
/*   99   */          mulsc r0._y__, c253.x, r0.y
                label L21
/*   10.1 */       exec
/*  100   */          add_sat r0.x___, r1.zzzz, r0.yyyy
/*  101   */          min r0.x___, r0.zzzz, r0.xxxx
                label L22
/*   11.0 */       alloc colors
/*   11.1 */       exece
/*  102   */          floors r0.x___, r0.x
/*  103   */          subsc r0.x___, c253.y, r0.x
/*  104   */          mad r4.___w, r0.xxxx, c252.xxxx, c251.xxxx
/*  105   */          max oC0, r4, r4
