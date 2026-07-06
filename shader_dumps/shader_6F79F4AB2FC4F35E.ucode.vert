/*    0.0 */       exec
/*    9   */          add r0._yzw, r0.xxxx, c254.xxyz
/*   10   */          mul r5.___w, c18.zzzz, c254.xxxx
              +       mulsc r1.__z_, c251.x, r0.y
/*   11   */          vfetch_full r6, r1.z, vf0, DataFormat=FMT_32_32_32_32_FLOAT, Stride=16, Signed=true, NumFormat=integer, PrefetchCount=8
/*   12   */          vfetch_mini r9, Offset=4, DataFormat=FMT_32_32_32_32_FLOAT, Signed=true, NumFormat=integer
/*   13   */          vfetch_full r2.zyxw, r1.z, vf0, Offset=8, DataFormat=FMT_32_32_32_32_FLOAT, Stride=16, Signed=true, NumFormat=integer, PrefetchCount=8
/*   14   */          vfetch_mini r5.xyz_, Offset=12, DataFormat=FMT_32_32_32_FLOAT, Signed=true, NumFormat=integer
/*    0.1 */       exec
/*   15   */          vfetch_mini r3, Offset=15, DataFormat=FMT_8_8_8_8
/*   16   */          serialize
                      mad r1.___w, r2.wwww, c19.zzzz, c19.wwww
/*   17   */          mul r7._yzw, r2.wwww, c12.xxzy
              +       muls r4.x___, r2.xw
/*   18   */          min r1._y__, r2.wwww, c17.zzzz
              +       maxs r0._y__, r2.ww
/*   19   */          mul r4._yzw, r2.wwww, c13.xxyz
              +       mulsc r1.x___, c18.x, r0.y
/*   20   */          mad r0._y__, r5.wwww, r2.wwww, r5.yyyy
/*    1.0 */       exec
/*   21   */          mad r0._y__, r0.yyyy, r2.wwww, r5.xxxx
/*   22   */          mad r2.x___, -r1.yyyy, c17.wwww, c252.yyyy
/*   23   */          min r1.x___, r1.xxxx, c251.yyyy
              +       truncs r1.__z_, r1.z
/*   24   */          add r8.xyz_, r9.zxyy, -c12.zxyy
              +       mulsc r2.___w, c251.z, r1.x
/*   25   */          mul r8.xyz_, r8.xyzz, c18.wwww
              +       maxs r0._, r2.xx
/*   26   */          max r0._y__, r0.yyyy, c252.xxxx
              +       muls_prev r2.x___, r9.w
/*    1.1 */       exec
/*   27   */          add r9, r4, c16.wxyz
              +       maxs r0._, r2.xx
/*   28   */          min r4.___w, r9.xxxx, r1.wwww
              +       muls_prev r7.x___, r1.y
/*   29   */          mul r4.xyz_, r9.yzww, r3.xyzz
              +       maxs r1._y__, r0.yy
/*   30   */          add r7, r7, r6.wxzy
              +       maxs r1.___w, r5.zz
/*   31   */          mul r4, r4, c8
              +       maxs r0._y__, r7.xx
/*   32   */          max r4, r4.wxyz, c14.wxyz
              +       mulsc r2.x___, c251.w, r0.y
/*    2.0 */       exec
/*   33   */          add r2.x__w, r2.xwww, c253.zwww
              +       mins r0._y__, r1.yw
/*   34   */          min r4, r4, c15.wxyz
              +       frcs r1._y__, r2.x
/*   35   */          mul r4, r4.yzwx, c255.xxxx
              +       maxs r0._, r2.ww
/*   36   */          mul r1.__zw, r1.yyyz, c252.zzzw
              +       muls_prev r1._y__, r1.x
/*   37   */          add r3.x_z_, r0.wzzz, -r1.wwww
              +       maxs r0._, -r1.ww
/*   38   */          add r6._yz_, r1.yyzz, c253.xxyy
              +       adds_prev r3._y__, r0.x
/*    2.1 */       exec
/*   39   */          seq r0.__z_, r3.xxxx, c252.xxxx
              +       cos r5.x___, r6.z
/*   40   */          sne r0.x___, r3.yyyy, c252.xxxx
              +       sin r5._y__, r6.z
/*   41   */          cndeq r0.x___, r3.xxxx, c252.yyyy, r0.xxxx
/*   42   */          cndeq r2.x__w, r3.zzzz, c252.yxxx, r0.zxxx
/*   43   */          mad r0.x_z_, r2.wxxx, c254.zzzz, c17.xyyy
/*   44   */          mul r9, r0.xzxz, r5.xxyy
              +       mulsc r6.x___, c9.x, r0.y
/*    3.0 */       exec
/*   45   */          add r1.__z_, r9.zzzz, r9.yyyy
              +       subs r1._y__, r9.xw
/*   46   */          mul r6.xyz_, r6.xxyy, r1.yzxx
              +       sgts r1.___w, -r_abs[0].x
/*   47   */          add r0.x___, r6.zzzz, c252.yyyy
              +       sgts r1.__z_, -r_abs[0].x
/*   48   */          mul r0.xyz_, r8.yxzz, r0.xxxx
              +       sgts r1._y__, -r_abs[0].x
/*   49   */          mad r7.xyz_, r0.xyzz, r1.xxxx, r7.yzww
/*   50   */          mul r0.xyz_, r7.yyyy, c2.zxyy
              +       sgts r1.x___, -r_abs[0].x
/*    3.1 */       exec    // PredicateClean=false
/*   51   */          mad r0.xyz_, r7.zzzz, c1.zxyy, r0.xyzz
/*   52   */          mad r0.xyz_, r7.xxxx, c0.zxyy, r0.xyzz
/*   53   */          add r5.__zw, r0.zzzy, c3.yyyx
              +       addsc r0.x___, c3.z, r0.x
/*   54   */          add r0._yz_, r5.zzww, r6.yyxx
              +       setp_ge r0._, c25.x
/*    4.0 */  (p0) exec
/*   55   */     (p0) mad r1, r0.xxxx, c23.zwxy, c24.zwxy
/*   56   */     (p0) mad r1, r0.yyyy, c22.wzxy, r1.yxzw
/*   57   */     (p0) mad r6, r0.zzzz, c21.zwxy, r1.yxzw
/*   58   */     (p0) max r1.___w, c25.xxxx, c25.xxxx
              +  (p0) rcp r0.___w, r6.y
/*   59   */     (p0) mul r1.x___, r6.zzzz, r0.wwww
/*   60   */     (p0) mul r1._y__, r6.wwww, -r0.wwww
/*    4.1 */  (p0) exec
/*   61   */     (p0) add r1.xy__, r1.xyyy, c252.yyyy
              +  (p0) maxs r0._, r6.xx
/*   62   */     (p0) mul r1.xy__, r1.xyyy, c254.xxxx
              +  (p0) muls_prev r1.__z_, r0.w
/*    5.0 */       exec    // PredicateClean=false
/*   63   */          max r0.___w, c20.zzzz, c20.zzzz
              +       setp_gt r0._, r3.w
/*   64   */     (p0) max r0.___w, -c20.zzzz, -c20.zzzz
              +  (p0) maxs r2.__z_, -r2.zz
/*   65   */          mul r5.___w, r2.wwww, r0.wwww
              +       mulsc r5.__z_, c20.w, r2.x
/*   66   */          mad r6, r0.xxxx, c6, c7
/*   67   */          mad r6, r0.yyyy, c5, r6
/*   68   */          add r2._yz_, r5.wwzz, r2.zzyy
              +       setp_eq r0._, c10.z
/*    5.1 */       exec
/*   69   */          mad r0, r0.zzzz, c4, r6
/*    6.0 */ (!p0) exec
/*   70   */    (!p0) cndeq r6._yz_, r3.xxyy, c254.xxww, c254.wwxx
/*   71   */    (!p0) cndeq r6.x___, r3.xxxx, c254.xxxx, r6.zzzz
/*   72   */    (!p0) cndeq r2.x__w, r3.zzzz, c254.wxxx, r6.xyyy
/*   73   */    (!p0) mul r3.___w, r2.wwww, -r5.yyyy
/*   74   */    (!p0) mul r3.xyz_, r2.wxxx, r5.xxyy
/*   75   */    (!p0) add r2.x__w, r3.zyyy, r3.xwww
/*    6.1 */ (!p0) exec
/*   76   */    (!p0) add r2.x__w, r2.xwww, c254.xxxx
/*    7.0 */       alloc position
/*    7.1 */       exec
/*   77   */          max oPos, r0, r0
/*    8.0 */       alloc interpolators
/*    8.1 */       exece
/*   78   */          max o2, r4, r4
/*   79   */          max o0, r2.yzwx, r2.yzwx
/*   80   */          max o1, r1, r1
