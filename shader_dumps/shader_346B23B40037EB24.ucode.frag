/*    0.0 */       exec
/*   12   */          tfetch2D r2, r0.xy, tf3, FetchValidOnly=false
/*   13   */          serialize
                      add r0.___w, c25.xxxx, -c254.wwww
/*   14   */          sge r3.xyz_, r2.zwww, c255.xyzz
              +       sgts r5.x___, -r_abs[0].x
/*   15   */          sge r1.__z_, r2.wwww, c253.wwww
              +       maxs r0._, c27.xx
/*   16   */          mul r1.x___, r3.zzzz, r1.zzzz
              +       adds_prev r0.__z_, -c254.w
/*   17   */          mad r1.___w, -r3.xxxx, c252.wwww, r2.zzzz
/*    0.1 */       exec    // PredicateClean=false
/*   18   */          mad r0.__z_, r0.zzzz, r3.yyyy, c254.wwww
/*   19   */          sge r1._y__, r1.wwww, c251.yyyy
              +       subsc r0.__z_, -c27.x, -r0.z
/*   20   */          mad r2.__z_, -r1.yyyy, c253.yyyy, r1.wwww
/*   21   */          mad r0.__z_, r0.zzzz, r3.zzzz, c27.xxxx
/*   22   */          add r1.___w, r0.zzzz, -c254.wwww
              +       setp_gt r0._, r_abs[1].x
/*   23   */     (p0) mad r2.___w, r0.wwww, r1.yyyy, c254.wwww
/*    1.0 */  (p0) exec
/*   24   */     (p0) mulsc r0.___w, c254.x, r2.y
/*   25   */     (p0) frcs r0.__z_, r0.w
/*   26   */     (p0) sge r1._y__, r0.zzzz, c252.yyyy
              +  (p0) subsc r2.x___, c254.w, r0.z
/*   27   */     (p0) mad r2.x___, r2.xxxx, r1.yyyy, r0.zzzz
/*   28   */     (p0) add r0.__z_, -r2.xxxx, r0.wwww
/*   29   */     (p0) mad r0.__z_, r0.zzzz, c254.zzzz, c251.zzzz
/*    1.1 */       exec
/*   30   */     (p0) sge r0.___w, r0.zzzz, c252.yyyy
              +  (p0) subsc r1._y__, c254.w, r0.z
/*   31   */     (p0) mad r2._y__, r1.yyyy, r0.wwww, r0.zzzz
/*   32   */    (!p0) sge r0.__z_, r2.zzzz, c253.zzzz
/*   33   */    (!p0) mad r2.___w, r0.wwww, r0.zzzz, c254.wwww
/*   34   */    (!p0) mad r0.___w, -r0.zzzz, c255.wwww, r2.zzzz
/*   35   */    (!p0) sge r0.__z_, r0.wwww, c251.xxxx
/*    2.0 */       exec
/*   36   */    (!p0) mulsc r5.x___, c26.x, r0.z
/*   37   */    (!p0) mad r2.__z_, -r0.zzzz, c252.zzzz, r0.wwww
/*   38   */          tfetch2D r1._x__, r0.xy, tf1, FetchValidOnly=false
/*   39   */          serialize
                      mul r8._yz_, r2.yyzz, c254.xxyy
/*   40   */          mul r7._yz_, r1.xxxx, r2.xxyy
              +       frcs r0.__z_, r8.y
/*   41   */          mul r8.x___, r0.zzzz, c254.xxxx
              +       mulsc_sat r8.___w, c21.x, r1.y
/*    2.1 */       exec
/*   42   */          floor r0.__zw, r8.xxxy
/*   43   */          mad r0.__zw, r0.zzzw, c254.zzzz, c251.zzzz
/*   44   */          mul r0.__zw, r0.wwwz, c252.wwww
/*   45   */          tfetch2D r6, r0.xy, tf4, FetchValidOnly=false
/*   46   */          tfetch2D r3.x_yz, r0.wz, tf2
/*   47   */          tfetch2D r4.wxyz, r0.xy, tf0, FetchValidOnly=false
/*    3.0 */       exec    // PredicateClean=false
/*   48   */          serialize
                      sgt r0.__z_, r4.yyyy, c253.wwww
/*   49   */          setp_ne_push r2._y__, c252.xxxx, r0.zzzz
/*   50   */          mul r7.x__w, r8.zwww, r1.xzzz
              +       subsc r0.__z_, c254.w, r1.x
/*   51   */          mad r3._y__, r7.wwww, r1.wwww, c254.wwww
/*   52   */          mad r3.x_zw, r0.zzzz, r3.xzzw, r7.yzzx
/*   53   */     (p0) max r2.x___, r0.xxxx, r0.xxxx
              +  (p0) rcp r0.__z_, c24.x
/*    3.1 */  (p0) exec
/*   54   */     (p0) max r1.___w, r0.yyyy, r0.yyyy
              +  (p0) rcp r0.___w, c23.x
/*   55   */     (p0) mul r5._yz_, r0.zzww, c49.xxxx
/*   56   */     (p0) add r2.__z_, -r5.yyyy, r0.yyyy
/*   57   */     (p0) add r1.__z_, -r5.zzzz, r0.xxxx
              +  (p0) maxs r0._, r5.yy
/*   58   */     (p0) add r0.___w, r5.zzzz, r0.xxxx
              +  (p0) adds_prev r0.__z_, r0.y
/*   59   */     (p0) tfetch2D r1.x___, r2.xz, tf1, FetchValidOnly=false
/*    4.0 */  (p0) exec
/*   60   */     (p0) tfetch2D r0.__x_, r0.xz, tf1, FetchValidOnly=false
/*   61   */     (p0) tfetch2D r1.__x_, r1.zw, tf1, FetchValidOnly=false
/*   62   */     (p0) tfetch2D r0.___x, r0.wy, tf1, FetchValidOnly=false
/*   63   */          serialize
                 (p0) max r0.___w, r1.yyyy, r0.wwww
/*   64   */     (p0) max r0.___w, r0.wwww, r1.zzzz
/*   65   */     (p0) max r0.__z_, r0.wwww, r0.zzzz
/*    4.1 */       exec    // PredicateClean=false
/*   66   */     (p0) max r0.__z_, r0.zzzz, r1.xxxx
/*   67   */     (p0) mulsc_sat r2.__z_, c20.x, r0.z
/*   68   */     (p0) sgt r0.__z_, r2.zzzz, c22.xxxx
/*   69   */          setp_eq_push r2._y__, r2.yyyy, r0.zzzz
/*   70   */     (p0) mul r0, r4, c50.xxxx
/*   71   */     (p0) mul r1, r0, c253.xxxx
/*    5.0 */       exec    // PredicateClean=false
/*   72   */     (p0) mad r0.x___, r2.zzzz, r1.xxxx, c254.wwww
/*   73   */     (p0) add r0._yzw, r1.yyzw, c254.wwww
/*   74   */     (p0) mad r0._yzw, r0.yyzw, r3.xxzw, -r3.xxzw
/*   75   */     (p0) mad r3.x_zw, r2.zzzz, r0.yzzw, r3.xzzw
/*   76   */          setp_inv r2._y__, r2.y
/*   77   */     (p0) sgt r0.__z_, c254.wwww, r6.xxxx
/*    5.1 */       exec    // PredicateClean=false
/*   78   */          setp_ne_push r2._y__, r2.yyyy, r0.zzzz
/*   79   */     (p0) sgt r7, -r_abs[0].xxxx, c252.xxxx
              +  (p0) maxs r0._, c48.xx
/*   80   */     (p0) sgt r1, -r_abs[0].xxxx, c252.xxxx
              +  (p0) adds_prev r2.x___, -c254.w
/*   81   */     (p0) mul r0.__z_, r5.zzzz, r2.xxxx
/*   82   */     (p0) mad r0.__z_, -r0.zzzz, c252.wwww, r0.xxxx
/*    6.0 */       loop i16, L15
                label L13
/*    6.1 */  (p0) exec
/*   83   */     (p0) mad r0.___w, r5.zzzz, r1.xxxx, r0.zzzz
/*   84   */     (p0) tfetch2D r4, r0.wy, tf4
/*   85   */          serialize
                 (p0) add r1._yzw, r4.yyzw, r1.wwzy
              +  (p0) addsc r1.x___, c254.w, r1.x
/*   86   */     (p0) max r7._yzw, r1.wwzy, r1.wwzy
              +  (p0) maxs r0._, r4.xx
/*   87   */     (p0) max r1._yzw, r1.wwzy, r1.wwzy
              +  (p0) adds_prev r7.x___, r7.x
/*    7.0 */ (!p0) endloop i16, L13
                label L15
/*    7.1 */  (p0) exec
/*   88   */     (p0) sgt r1, -r_abs[0].xxxx, c252.xxxx
/*   89   */     (p0) mul r0.___w, r5.yyyy, r2.xxxx
              +  (p0) maxs r6.x___, r1.ww
/*   90   */     (p0) sgt r4, -r_abs[0].xxxx, c252.xxxx
              +  (p0) rcp r2.x___, c48.x
/*   91   */     (p0) max r8._y__, r1.xxxx, r1.xxxx
              +  (p0) maxs r8.x___, r1.yy
/*   92   */     (p0) mul r1, r2.xxxx, r7.xwzy
              +  (p0) maxs r0.__z_, r1.zz
/*   93   */     (p0) mad r0._y__, -r0.wwww, c252.wwww, r0.yyyy
/*    8.0 */       loop i16, L19
                label L17
/*    8.1 */  (p0) exec
/*   94   */     (p0) mad r0.___w, r5.yyyy, r0.zzzz, r0.yyyy
/*   95   */     (p0) tfetch2D r7, r0.xw, tf4
/*   96   */          serialize
                 (p0) max r6.__zw, r4.zzzw, r4.zzzw
              +  (p0) maxs r6._y__, r8.yy
/*   97   */     (p0) add r6, r7.xwyz, r6.xywz
/*   98   */     (p0) max r8.xy__, r6.yyyy, r6.yyyy
/*   99   */     (p0) max r4, r6.wzwz, r6.wzwz
              +  (p0) addsc r0.__z_, c254.w, r0.z
/*    9.0 */ (!p0) endloop i16, L17
                label L19
/*    9.1 */  (p0) exec
/*  100   */     (p0) max r8._yz_, r4.xxyy, r4.xxyy
              +  (p0) maxs r8.___w, r6.xx
/*  101   */     (p0) mad r0, r2.xxxx, r8.wzyx, r1
/*  102   */     (p0) mul r0, r0.wzxy, c50.xxxx
/*  103   */     (p0) mul r1, r0.ywxz, c253.xxxx
/*  104   */     (p0) mad r0.x___, r2.zzzz, r1.zzzz, c254.wwww
/*  105   */     (p0) add r0._yzw, r1.yyxw, c254.wwww
/*   10.0 */       exec    // PredicateClean=false
/*  106   */     (p0) mad r0._yzw, r0.yyzw, r3.zzwx, -r3.zzwx
/*  107   */     (p0) mad r3.x_zw, r2.zzzz, r0.wyyz, r3.xzzw
/*  108   */          setp_inv r2._y__, r2.y
/*  109   */     (p0) mad r0.x___, r2.zzzz, -r6.wwww, c254.wwww
/*  110   */     (p0) add r0._yzw, -r6.xxyz, c254.wwww
/*  111   */     (p0) add r0._yzw, r0.yyzw, -r3.xxzw
/*   10.1 */       exec    // PredicateClean=false
/*  112   */     (p0) mad r3.x_zw, r2.zzzz, r0.yzzw, r3.xzzw
/*  113   */          setp_pop r2._y__, r2.y
/*  114   */          setp_pop r2._y__, r2.y
/*  115   */          setp_inv r2._y__, r2.y
/*  116   */     (p0) maxs r0.x___, c254.ww
/*   11.0 */       alloc colors
/*   11.1 */       exece
/*  117   */          mul r0._yz_, r3.xxyy, r2.wwww
/*  118   */          mul r0.x___, r0.zzzz, r0.xxxx
/*  119   */          mul r0._yzw, r0.yyzz, r3.yyzw
/*  120   */          add oC0, r0.yzwx, r5.xxxx
