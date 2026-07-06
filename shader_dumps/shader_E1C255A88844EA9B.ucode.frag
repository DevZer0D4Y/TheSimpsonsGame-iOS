/*    0.0 */       exec    // PredicateClean=false
/*   16   */          tfetch2D r8.xzyw, r0.xy, tf2
/*   17   */          serialize
                      max r9.xyz_, -r4.xyzz, -r4.xyzz
/*   18   */          add r7.xy_w, -r1.xyzz, c4.xyzz
/*   19   */          dp3 r1.__z_, r7.wxyy, r7.wxyy
              +       maxs_sat r1.x___, r4.yy
/*   20   */          sgt r1._y__, c44.wwww, c255.xxxx
              +       rsq r1.__z_, r_abs[1].z
/*   21   */          mul r7.xy_w, r7.xyww, r1.zzzz
              +       setp_ne r0._, r1.y
/*    0.1 */ (!p0) jmp L3
/*    1.0 */       exec
/*   22   */          max r9.xyz_, c36.xyzz, c36.xyzz
                label L3
/*    1.1 */       exec    // PredicateClean=false
/*   23   */          dp3 r1._y__, c44.zxyy, c44.zxyy
/*   24   */          sqrt r1._y__, r_abs[1].y
/*   25   */          sgt r1._y__, r1.yyyy, c252.yyyy
/*   26   */          setp_ne r0._, r1.y
/*    2.0 */ (!p0) jmp L6
/*    2.1 */       exec
/*   27   */          max r9.xyz_, c44.xyzz, c44.xyzz
                label L6
/*    3.0 */       exec
/*   28   */          tfetch2D r10, r0.xy, tf3
/*   29   */          serialize
                      mad r10, r10, c252.xxxx, c255.zzzz
/*   30   */          dp4 r0._y__, r10.wzxy, r10.wzxy
/*   31   */          dp3 r5.___w, r9.zxyy, r9.zxyy
              +       rsq r0._y__, r_abs[0].y
/*   32   */          mul r1._yzw, r10.yyzx, r0.yyyy
/*   33   */          mul r6.xyz_, r1.yyyy, r6.xyzz
              +       mulsc r0._y__, c46.x, r1.z
/*    3.1 */       exec
/*   34   */          mad r1._yzw, r1.wwww, r5.xxyz, r6.xxyz
/*   35   */          mad r1._yzw, r0.yyyy, r4.xxyz, r1.yyzw
/*   36   */          dp3 r0._y__, r1.wyzz, r1.wyzz
              +       rsq r5.x___, r_abs[5].w
/*   37   */          mul r5.xyz_, r5.xxxx, r9.xyzz
              +       rsq r0._y__, r_abs[0].y
/*   38   */          mul r1._yzw, r1.yyzw, r0.yyyy
/*   39   */          dp3 r0._y__, r1.wyzz, r5.zxyy
/*    4.0 */       exec
/*   40   */          dp3 r4.___w, r7.wxyy, r7.wxyy
              +       adds r0._y__, r_abs[0].yy
/*   41   */          mad r6.xyz_, r1.yzww, r0.yyyy, -r5.xyzz
/*   42   */          dp3 r0._y__, r6.zxyy, r6.zxyy
              +       rsq r4.___w, r_abs[4].w
/*   43   */          mul r5.xyz_, r7.xyww, r4.wwww
              +       rsq r0._y__, r_abs[0].y
/*   44   */          mul r6.xyz_, r6.xyzz, r0.yyyy
/*   45   */          dp3 r0._y__, r6.zxyy, r5.zxyy
/*    4.1 */       exec    // PredicateClean=false
/*   46   */          log r0._y__, r_abs[0].y
/*   47   */          mulsc r0._y__, c50.x, r0.y
/*   48   */          mul r4.___w, r8.wwww, c47.xxxx
              +       exp r0._y__, r0.y
/*   49   */          mul r0._y__, r4.wwww, r0.yyyy
/*   50   */          sgt r4.___w, c46.zzzz, r0.yyyy
/*   51   */          setp_ne r0._, r4.w
/*    5.0 */       exec    // PredicateClean=false
/*   52   */     (p0) sgts r0._y__, -r_abs[0].x
/*   53   */          dp3_sat r1._y__, r1.wyzz, r9.zxyy
              +       subsc r4.___w, c255.w, r1.x
/*   54   */          sgt r1.x___, r1.yyyy, c45.xxxx
              +       maxs r0.x___, c45.ww
/*   55   */          dp3_sat r1.___w, r4.zxyy, c36.zxyy
              +       setp_ne r0._, r1.x
/*   56   */     (p0) adds r0.x___, c45.ww
/*   57   */          sgt r1.x___, r1.yyyy, c45.yyyy
/*    5.1 */       exec    // PredicateClean=false
/*   58   */          setp_ne r0._, r1.x
/*   59   */     (p0) addsc r0.x___, c45.w, r0.x
/*   60   */          sgt r1.x___, r1.yyyy, c45.zzzz
/*   61   */          setp_ne r0._, r1.x
/*   62   */     (p0) addsc r0.x___, c45.w, r0.x
/*   63   */          sgt r4._y__, c47.yyyy, c255.xxxx
/*    6.0 */       exec
/*   64   */          sge r5.x___, c47.zzzz, c255.xxxx
/*   65   */          sge r9.___w, c252.zzzz, r1.yyyy
              +       maxs r4.x___, c255.ww
/*   66   */          sge r6.x___, r7.zzzz, c251.yyyy
              +       mulsc r1.x___, c46.y, r0.x
/*   67   */          frc r7._yz_, r0.zzww
              +       mulsc r0.x___, c46.w, r0.y
/*   68   */          mad_sat r9.xyz_, r1.xxxx, r8.xyzz, r0.xxxx
/*   69   */          min r6._yz_, r9.xxzz, c251.zzzz
              +       adds r0.x___, r7.zz
/*    6.1 */       exec
/*   70   */          sge r6.___w, r0.xxxx, c255.wwww
              +       snes r5._y__, c49.z
/*   71   */          mul r6, r6.xzyw, c255.xyyz
              +       maxs r0._, c49.zz
/*   72   */          floor r5.__zw, r6.yyyz
              +       adds_prev r1.__z_, -c40.x
/*   73   */          mul r1.xy__, r9.ywww, c254.zwww
              +       maxs r0._, r0.xx
/*   74   */          mul r0.xy__, r1.yzzz, r5.xyyy
              +       adds_prev r7.x___, r6.w
/*   75   */          mul r5.xy__, r7.xyyy, c254.xyyy
              +       addsc r0.__z_, c40.x, r0.y
/*    7.0 */       exec    // PredicateClean=false
/*   76   */          floor r5.xy__, r5.yxxx
              +       maxs r0._, r6.xx
/*   77   */          mul r6._yz_, r5.yyzz, c255.yyyy
              +       adds_prev r0.x___, r0.x
/*   78   */          add r6.x___, r0.zzzz, r6.yyyy
              +       maxs r0._, r0.xx
/*   79   */          add r0.xy__, r6.xzzz, r5.xwww
              +       adds_prev r1.__z_, r1.x
/*   80   */          mul r1.xy__, r0.xyyy, c253.zwww
              +       setp_ne r0._, r4.y
/*    7.1 */ (!p0) jmp L30
/*    8.0 */       exec    // PredicateClean=false
/*   81   */          max r0.__z_, c255.wwww, c255.wwww
              +       rcp r0._y__, r2.w
/*   82   */          mul_sat r0.x___, r0.yyyy, r2.zzzz
/*   83   */          mul r0._y_w, r0.yyyy, r2.xxyy
              +       setp_gt r0._, c31.x
/*   84   */          mad r0._y_w, r0.yyww, c253.xxyy, c255.xxxx
/*    8.1 */ (!p0) jmp L22
/*    9.0 */       exec
/*   85   */          tfetch2D r5.y___, r0.yw, tf1
/*   86   */          tfetch2D r5._z__, r0.yw, tf1, OffsetX=1
/*   87   */          tfetch2D r4.z___, r0.yw, tf1, OffsetX=1, OffsetY=-1
/*   88   */          tfetch2D r4._x__, r0.yw, tf1, OffsetX=-1, OffsetY=-1
/*   89   */          tfetch2D r4.__y_, r0.yw, tf1, OffsetY=-1
/*   90   */          tfetch2D r2.x___, r0.yw, tf1, OffsetX=-1
/*    9.1 */       exec
/*   91   */          tfetch2D r2._z__, r0.yw, tf1, OffsetX=1, OffsetY=1
/*   92   */          tfetch2D r2.__x_, r0.yw, tf1, OffsetX=-1, OffsetY=1
/*   93   */          tfetch2D r2.___y, r0.yw, tf1, OffsetY=1
/*   94   */          serialize
                      mul r0.__zw, r0.yyyw, c252.wwww
              +       subsc r0.x___, c255.w, r0.x
/*   95   */          sge r2, r0.xxxx, r2
/*   96   */          sge r4.xyz_, r0.xxxx, r4.xyzz
              +       frcs r0._y__, r0.w
/*   10.0 */       exec
/*   97   */          add r6.xyz_, r2.yzww, -r4.xyzz
              +       frcs r0.___w, r0.z
/*   98   */          mad r4.xyz_, r6.xyzz, r0.yyyy, r4.xyzz
/*   99   */          sge r0.xy__, r0.xxxx, r5.xyyy
              +       maxs r0._, r4.yy
/*  100   */          add r0._yz_, r4.xxzz, r0.yyxx
              +       adds_prev r0.x___, r2.x
/*  101   */          add r2.x___, r0.yyyy, -r0.xxxx
/*  102   */          mad r0.x___, r2.xxxx, r0.wwww, r0.xxxx
/*   10.1 */       exec
/*  103   */          add r0._y__, r0.xxxx, r0.zzzz
/*  104   */          mulsc r0.__z_, c255.x, r0.y
                label L22
/*   11.0 */       exec
/*  105   */          floor r2.__z_, -r1.wwww
              +       maxs r0._y__, c255.ww
/*  106   */          add r2.x___, -c30.yyyy, c255.wwww
              +       rcp r1.___w, r3.w
/*  107   */          mul_sat r0.x___, r1.wwww, r3.zzzz
              +       floors r0.___w, r4.w
/*  108   */          mad_sat r0.__z_, r2.xxxx, r0.zzzz, c30.yyyy
/*  109   */          mul r2.xy__, r1.wwww, r3.xyyy
              +       maxs r0._, r0.zz
/*  110   */          mul r2.xy__, r2.xyyy, c253.xyyy
              +       adds_prev_sat r0.__z_, r0.w
/*   11.1 */       exec    // PredicateClean=false
/*  111   */          add r2.xyz_, r2.xyzz, c255.xxww
              +       setp_gt r0._, c31.x
/*   12.0 */ (!p0) jmp L29
/*   12.1 */       exec
/*  112   */          tfetch2D r0._y__, r2.xy, tf0
/*  113   */          tfetch2D r0.___z, r2.xy, tf0, OffsetX=1
/*  114   */          tfetch2D r4.z___, r2.xy, tf0, OffsetX=1, OffsetY=-1
/*  115   */          tfetch2D r4._x__, r2.xy, tf0, OffsetX=-1, OffsetY=-1
/*  116   */          tfetch2D r4.__y_, r2.xy, tf0, OffsetY=-1
/*  117   */          tfetch2D r3.x___, r2.xy, tf0, OffsetX=-1
/*   13.0 */       exec
/*  118   */          tfetch2D r3._z__, r2.xy, tf0, OffsetX=1, OffsetY=1
/*  119   */          tfetch2D r3.__x_, r2.xy, tf0, OffsetX=-1, OffsetY=1
/*  120   */          tfetch2D r3.___y, r2.xy, tf0, OffsetY=1
/*  121   */          serialize
                      mul r6.xy__, r2.xyyy, c252.wwww
              +       subsc r0.x___, c255.w, r0.x
/*  122   */          sge r3, r0.xxxx, r3
/*  123   */          sge r4.xyz_, r0.xxxx, r4.xyzz
              +       frcs r2.___w, r6.y
/*   13.1 */       exec
/*  124   */          add r5.xyz_, r3.yzww, -r4.xyzz
              +       frcs r1.___w, r6.x
/*  125   */          mad r4.xyz_, r5.xyzz, r2.wwww, r4.xyzz
/*  126   */          sge r0.xy__, r0.xxxx, r0.ywww
              +       maxs r0._, r4.yy
/*  127   */          add r0.xy__, r4.xzzz, r0.yxxx
              +       adds_prev r0.___w, r3.x
/*  128   */          add r2.___w, r0.xxxx, -r0.wwww
/*  129   */          mad r0.___w, r2.wwww, r1.wwww, r0.wwww
/*   14.0 */       exec
/*  130   */          add r0._y__, r0.wwww, r0.yyyy
/*  131   */          mulsc r0._y__, c255.x, r0.y
                label L29
/*   14.1 */       exec
/*  132   */          add_sat r0.x___, r2.zzzz, r0.yyyy
/*  133   */          min r4.x___, r0.zzzz, r0.xxxx
                label L30
/*   15.0 */       alloc colors
/*   15.1 */       exece
/*  134   */          floors r0.x___, r4.x
/*  135   */          subsc r0.x___, c255.w, r0.x
/*  136   */          mad r1.___w, r0.xxxx, c251.wwww, c251.xxxx
/*  137   */          max oC0, r1, r1
