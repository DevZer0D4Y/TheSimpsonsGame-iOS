/*    0.0 */       exec    // PredicateClean=false
/*    7   */          vfetch_full r1.0001, r0.x, vf0, DataFormat=FMT_8_8_8_8, Stride=9, PrefetchCount=7
/*    8   */          vfetch_mini r3.1000, DataFormat=FMT_8_8_8_8
/*    9   */          vfetch_mini r0._xzy, DataFormat=FMT_32_32_32_FLOAT, Signed=true, NumFormat=integer
/*   10   */          vfetch_mini r2.xy__, Offset=5, DataFormat=FMT_32_32_FLOAT, Signed=true, NumFormat=integer
/*   11   */          serialize
                      setp_ne r0._, c40.x
/*    0.1 */ (!p0) jmp L6
/*    1.0 */       exec
/*   12   */          cndeq r4, c254.xxxy, r0.zyww, c254.yyyy
/*   13   */          mul r6, r3, c254.zzzz
/*   14   */          maxas r0._, r6.xx
/*   15   */          mul r0, r1.wwww, c[52+a0].wxyz
/*   16   */          mul r5, r1.wwww, c[53+a0].wxyz
/*   17   */          mul r3, r1.wwww, c[54+a0].wxyz
              +       maxas r0._, r6.yy
/*    1.1 */       exec
/*   18   */          mad r3, r1.zzzz, c[54+a0].wzxy, r3.xwyz
/*   19   */          mad r5, r1.zzzz, c[53+a0].wzxy, r5.xwyz
/*   20   */          mad r0, r1.zzzz, c[52+a0].wzxy, r0.xwyz
/*   21   */          maxas r0._, r6.zz
/*   22   */          mad r0, r1.yyyy, c[52+a0].xwyz, r0.zxwy
/*   23   */          mad r5, r1.yyyy, c[53+a0].xwyz, r5.zxwy
/*    2.0 */       exec
/*   24   */          mad r3, r1.yyyy, c[54+a0].xwyz, r3.zxwy
/*   25   */          maxas r0._, r6.ww
/*   26   */          mad r3, r1.xxxx, c[54+a0].yxwz, r3.zxyw
/*   27   */          mad r5, r1.xxxx, c[53+a0].yxwz, r5.zxyw
/*   28   */          mad r0, r1.xxxx, c[52+a0].yxwz, r0.zxyw
/*   29   */          dp4 r0._y__, r0.wyxz, r4
/*    2.1 */       exec
/*   30   */          dp4 r0.___w, r5.wyxz, r4
/*   31   */          dp4 r0.__z_, r3.wyxz, r4
                label L6
/*    3.0 */       alloc position
/*    3.1 */       exec
/*   32   */          mul r6, c15.wyxz, c3.wwww
              +       sges r1.___w, r_abs[0].x
/*   33   */          mul r5, c15.wyxz, c2.wwww
              +       subsc r1.x___, -c255.x, -r0.w
/*   34   */          mul r4, c15.wyxz, c1.wwww
              +       maxs r1._y__, r0.ww
/*   35   */          mul r3, c15.wyxz, c0.wwww
              +       maxs r1.__z_, c255.xx
/*   36   */          sge r0.___w, r0.wwww, c254.wwww
              +       mins r0.x___, r1.yz
/*   37   */          mad r3, c14.wyzx, c0.zzzz, r3.xywz
/*    4.0 */       exec
/*   38   */          mad r4, c14.wyzx, c1.zzzz, r4.xywz
/*   39   */          mad r5, c14.wyzx, c2.zzzz, r5.xywz
/*   40   */          mad r6, c14.wyzx, c3.zzzz, r6.xywz
/*   41   */          mad r0.x___, r1.xxxx, r0.wwww, r0.xxxx
/*   42   */          cndeq r1.xyz_, c254.yyxx, r0.xxxx, r0.zyzz
/*   43   */          mad r0, c13.ywxz, c3.yyyy, r6.yxwz
/*    4.1 */       exec
/*   44   */          mad r5, c13.ywxz, c2.yyyy, r5.yxwz
/*   45   */          mad r4, c13.ywxz, c1.yyyy, r4.yxwz
/*   46   */          mad r3, c13.ywxz, c0.yyyy, r3.yxwz
/*   47   */          mad r3, c12.yzwx, c0.xxxx, r3.xwyz
/*   48   */          mad r4, c12.yzwx, c1.xxxx, r4.xwyz
/*   49   */          mad r5, c12.yzwx, c2.xxxx, r5.xwyz
/*    5.0 */       exec
/*   50   */          mad r0, c12.yzwx, c3.xxxx, r0.xwyz
/*   51   */          dp4 r0.___w, r0.ywxz, r1
/*   52   */          dp4 r0.__z_, r5.ywxz, r1
/*   53   */          dp4 r0._y__, r4.ywxz, r1
/*   54   */          dp4 r0.x___, r3.ywxz, r1
/*   55   */          max oPos, r0, r0
/*    5.1 */       alloc interpolators
/*    6.0 */       exece
/*   56   */          max r0._, c0, c0
/*   57   */          max r0._, c0, c0
/*    6.1 */       cnop
