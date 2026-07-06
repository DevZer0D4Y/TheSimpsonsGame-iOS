/*    0.0 */       exec
/*    7   */          vfetch_full r6.0001, r0.x, vf0, DataFormat=FMT_8_8_8_8, Stride=10, PrefetchCount=3
/*    8   */          vfetch_mini r4.1000, DataFormat=FMT_8_8_8_8
/*    9   */          vfetch_mini r7.000_, DataFormat=FMT_8_8_8_8
/*   10   */          vfetch_mini r3.000_, DataFormat=FMT_8_8_8_8
/*   11   */          vfetch_mini r2.000_, DataFormat=FMT_8_8_8_8
/*   12   */          vfetch_mini r1.000_, DataFormat=FMT_8_8_8_8
/*    0.1 */       exec
/*   13   */          vfetch_mini r8.000_, DataFormat=FMT_8_8_8_8
/*   14   */          vfetch_mini r5.xyz_, DataFormat=FMT_32_32_32_FLOAT, Signed=true, NumFormat=integer
/*   15   */          vfetch_mini r0.000_, DataFormat=FMT_8_8_8_8
/*   16   */          serialize
                      max r0._, c0, c0
/*    1.0 */       cjmp !b0, L10
/*    1.1 */       exec    // PredicateClean=false
/*   17   */          sgt r0.___w, c37.wwww, c255.zzzz
/*   18   */          setp_ne r0._, r0.w
/*    2.0 */ (!p0) jmp L6
/*    2.1 */       exec
/*   19   */          mad r5.xyz_, r7.xyzz, c36.xxxx, r5.xyzz
/*   20   */          mad r3.xyz_, r3.xyzz, c36.yyyy, r5.xyzz
/*   21   */          mad r2.xyz_, r2.xyzz, c36.zzzz, r3.xyzz
/*   22   */          mad r1.xyz_, r1.xyzz, c36.wwww, r2.xyzz
/*   23   */          mad r1.xyz_, r8.xyzz, c37.xxxx, r1.xyzz
/*   24   */          mad r5.xyz_, r0.xyzz, c37.yyyy, r1.xyzz
                label L6
/*    3.0 */       exec
/*   25   */          cndeq r0, c255.xxxy, r5.zxyy, c255.yyyy
/*   26   */          mul r4, r4, c255.wwww
/*   27   */          maxas r0._, r4.xx
/*   28   */          mul r3, r6.wwww, c[52+a0].wxyz
/*   29   */          mul r2, r6.wwww, c[53+a0].wxyz
/*   30   */          mul r1, r6.wwww, c[54+a0].wxyz
              +       maxas r0._, r4.yy
/*    3.1 */       exec
/*   31   */          mad r1, r6.zzzz, c[54+a0].wzxy, r1.xwyz
/*   32   */          mad r2, r6.zzzz, c[53+a0].wzxy, r2.xwyz
/*   33   */          mad r3, r6.zzzz, c[52+a0].wzxy, r3.xwyz
/*   34   */          maxas r0._, r4.zz
/*   35   */          mad r3, r6.yyyy, c[52+a0].xwyz, r3.zxwy
/*   36   */          mad r2, r6.yyyy, c[53+a0].xwyz, r2.zxwy
/*    4.0 */       exec
/*   37   */          mad r1, r6.yyyy, c[54+a0].xwyz, r1.zxwy
/*   38   */          maxas r0._, r4.ww
/*   39   */          mad r1, r6.xxxx, c[54+a0].yxwz, r1.zxyw
/*   40   */          mad r2, r6.xxxx, c[53+a0].yxwz, r2.zxyw
/*   41   */          mad r3, r6.xxxx, c[52+a0].yxwz, r3.zxyw
/*   42   */          dp4 r5.x___, r3.wyxz, r0
/*    4.1 */       exec
/*   43   */          dp4 r5._y__, r2.wyxz, r0
/*   44   */          dp4 r5.__z_, r1.wyxz, r0
                label L10
/*    5.0 */       alloc position
/*    5.1 */       exec
/*   45   */          cndeq r0, c255.xxxy, r5.zxyy, c255.yyyy
/*   46   */          dp4 oPos.x___, c0.zxyw, r0
/*   47   */          dp4 oPos._y__, c1.zxyw, r0
/*   48   */          dp4 oPos.__z_, c2.zxyw, r0
/*   49   */          dp4 oPos.___w, c3.zxyw, r0
/*    6.0 */       alloc interpolators
/*    6.1 */       exece
