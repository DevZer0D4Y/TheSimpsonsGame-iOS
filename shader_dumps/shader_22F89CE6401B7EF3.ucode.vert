/*    0.0 */       exec
/*    7   */          vfetch_full r3.xyz1, r0.x, vf0, DataFormat=FMT_32_32_32_FLOAT, Stride=9, Signed=true, NumFormat=integer, PrefetchCount=7
/*    8   */          vfetch_mini r0._xyz, Offset=3, DataFormat=FMT_2_10_10_10, Signed=true
/*    9   */          vfetch_mini r6.zyxw, Offset=4, DataFormat=FMT_8_8_8_8
/*   10   */          vfetch_mini r2.xy__, Offset=5, DataFormat=FMT_32_32_FLOAT, Signed=true, NumFormat=integer
/*   11   */          vfetch_full r2.__xy, r0.x, vf0, Offset=7, DataFormat=FMT_32_32_FLOAT, Stride=9, Signed=true, NumFormat=integer
/*   12   */          serialize
                      sgt r4.___w, c46.wwww, c255.zzzz
/*    0.1 */       exec
/*   13   */          dp4 r1.x___, c0.zxyw, r3.zxyw
/*   14   */          dp4 r1._y__, c1.zxyw, r3.zxyw
/*   15   */          dp4 r1.__z_, c2.zxyw, r3.zxyw
/*   16   */          dp4 r1.___w, c3.zxyw, r3.zxyw
/*   17   */          dp3 r5.x___, r0.wyzz, c12.zxyy
/*   18   */          dp3 r5._y__, r0.wyzz, c13.zxyy
/*    1.0 */       exec
/*   19   */          dp3 r5.__z_, r0.wyzz, c14.zxyy
/*   20   */          dp4 r0.x___, c15.zxyw, r3.zxyw
/*   21   */          dp4 r4.__z_, c14.zxyw, r3.zxyw
/*   22   */          dp4 r4.x___, c12.zxyw, r3.zxyw
/*   23   */          dp4 r4._y__, c13.zxyw, r3.zxyw
/*   24   */          cndeq r7, c255.xyyy, r0.xxxx, r4.zzxy
/*    1.1 */       exec
/*   25   */          dp4 r0.x___, r7.yzwx, c26.zxyw
/*   26   */          dp4 r0._y__, r7.yzwx, c27.zxyw
/*   27   */          dp4 r0.__z_, r7.yzwx, c28.zxyw
/*   28   */          dp4 r0.___w, r7.yzwx, c29.zxyw
/*   29   */          dp4 r3.x___, r7.yzwx, c22.zxyw
/*   30   */          dp4 r3._y__, r7.yzwx, c23.zxyw
/*    2.0 */       exec    // PredicateClean=false
/*   31   */          dp4 r3.__z_, r7.yzwx, c24.zxyw
/*   32   */          dp4 r3.___w, r7.yzwx, c25.zxyw
              +       setp_ne r0._, r4.w
/*    2.1 */ (!p0) jmp L9
/*    3.0 */       exec    // PredicateClean=false
/*   33   */          sgt r4.___w, r_abs[5].yyyy, r_abs[5].xxxx
/*   34   */          max r5.___w, r_abs[5].xxxx, r_abs[5].xxxx
              +       setp_ne r0._, r4.w
/*   35   */    (!p0) maxs r4.___w, r4.yy
/*   36   */     (p0) max r4.___w, r4.xxxx, r4.xxxx
              +  (p0) maxs r5.___w, r_abs[5].yy
/*   37   */          sgt r5.___w, r_abs[5].zzzz, r5.wwww
/*   38   */          setp_ne r0._, r5.w
/*    3.1 */  (p0) exec
/*   39   */     (p0) max r4.__zw, r4.yyyx, r4.yyyx
/*    4.0 */       jmp L10
                label L9
/*    4.1 */       exec
/*   40   */          max r4.__zw, r2.yyyx, r2.yyyx
                label L10
/*    5.0 */       alloc position
/*    5.1 */       exec
/*   41   */          max oPos, r1, r1
/*    6.0 */       alloc interpolators
/*    6.1 */       exece
/*   42   */          max o4.xy__, r4.wzzz, r4.wzzz
/*   43   */          max o3.xyz_, r5.xyzz, r5.xyzz
/*   44   */          max o0, r2, r2
/*   45   */          max o1, r0, r0
/*   46   */          max o2, r3, r3
/*   47   */          max o5, r6, r6
