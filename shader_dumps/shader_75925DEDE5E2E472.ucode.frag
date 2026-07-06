/*    0.0 */       exec    // PredicateClean=false
/*   10   */          tfetch2D r6, r0.zw, tf1
/*   11   */          tfetch2D r7, r0.xy, tf0
/*   12   */          serialize
                      sgt r2.___w, c255.yyyy, c44.xxxx
/*   13   */          mul r0.__z_, r7.wwww, c44.yyyy
              +       maxs_sat r0.___w, r5.ww
/*   14   */          mul r1.___w, r6.wwww, c44.zzzz
              +       setp_ne r0._, r2.w
/*    0.1 */ (!p0) jmp L4
/*    1.0 */       exec
/*   15   */          mul r5.xyz_, r1.wwww, r6.xyzz
              +       subsc r2.___w, c254.x, r0.z
/*   16   */          mul r5.xyz_, r5.xyzz, r2.wwww
/*   17   */          mad r7.xyz_, r0.zzzz, r7.xyzz, r5.xyzz
/*    1.1 */       jmp L7
                label L4
/*    2.0 */       exec    // PredicateClean=false
/*   18   */          sgt r2.___w, c255.zzzz, c44.xxxx
/*   19   */          setp_ne r0._, r2.w
/*    2.1 */ (!p0) jmp L7
/*    3.0 */       exec
/*   20   */          mul r5.xyz_, r1.wwww, r6.xyzz
/*   21   */          mad r7.xyz_, r0.zzzz, r7.xyzz, r5.xyzz
                label L7
/*    3.1 */       exec    // PredicateClean=false
/*   22   */          sgt r1.___w, c42.wwww, c255.yyyy
/*   23   */          add r5.xyz_, -r1.xyzz, c4.xyzz
/*   24   */          dp3 r2.___w, r5.zxyy, r5.zxyy
/*   25   */          max r1.xyz_, -r2.xyzz, -r2.xyzz
              +       rsq r2.___w, r_abs[2].w
/*   26   */          mul r5.xyz_, r5.xyzz, r2.wwww
              +       setp_ne r0._, r1.w
/*    4.0 */ (!p0) jmp L10
/*    4.1 */       exec
/*   27   */          max r1.xyz_, c36.xyzz, c36.xyzz
                label L10
/*    5.0 */       exec    // PredicateClean=false
/*   28   */          dp3 r1.___w, c42.zxyy, c42.zxyy
/*   29   */          sqrt r1.___w, r_abs[1].w
/*   30   */          sgt r1.___w, r1.wwww, c255.wwww
/*   31   */          setp_ne r0._, r1.w
/*    5.1 */ (!p0) jmp L13
/*    6.0 */       exec
/*   32   */          max r1.xyz_, c42.xyzz, c42.xyzz
                label L13
/*    6.1 */       exec
/*   33   */          mul r0.xy__, r0.xyyy, c43.yyyy
/*   34   */          tfetch2D r6, r0.xy, tf2
/*    7.0 */       alloc colors
/*    7.1 */       exec
/*   35   */          mad r6, r6.zxyw, c255.xxxx, c254.yyyy
/*   36   */          dp4 r0.x___, r6.wxyz, r6.wxyz
/*   37   */          dp3 r3.___w, r1.zxyy, r1.zxyy
              +       rsq r0.x___, r_abs[0].x
/*   38   */          mul r0.xyz_, r6.yxzz, r0.xxxx
/*   39   */          mul r4.xyz_, r0.zzzz, r4.xyzz
              +       mulsc r1.___w, c43.x, r0.y
/*   40   */          mad r0.xyz_, r0.xxxx, r3.xyzz, r4.xyzz
/*    8.0 */       exec
/*   41   */          mad r2.xyz_, r1.wwww, r2.xyzz, r0.xyzz
/*   42   */          dp3 r1.___w, r2.zxyy, r2.zxyy
              +       rsq r0.x___, r_abs[3].w
/*   43   */          mul r0.xyz_, r0.xxxx, r1.xyzz
              +       rsq r1.x___, r_abs[1].w
/*   44   */          mul r1.xyz_, r2.xyzz, r1.xxxx
/*   45   */          dp3 r1.___w, r1.zxyy, r0.zxyy
/*   46   */          dp3 r2.___w, r5.zxyy, r5.zxyy
              +       adds r1.___w, r_abs[1].ww
/*    8.1 */       exec
/*   47   */          mad r2.xyz_, r1.xyzz, r1.wwww, -r0.xyzz
/*   48   */          dp3 r1.___w, r2.zxyy, r2.zxyy
              +       rsq r0.x___, r_abs[2].w
/*   49   */          mul r0.xyz_, r5.xyzz, r0.xxxx
              +       rsq r1.___w, r_abs[1].w
/*   50   */          mul r2.xyz_, r2.xyzz, r1.wwww
/*   51   */          dp3 r0.x___, r2.zxyy, r0.zxyy
/*   52   */          dp3 r0.__z_, r5.zxyy, r1.zxyy
              +       log r0._y__, r_abs[0].x
/*    9.0 */       exece
/*   53   */          mulsc r0.x___, c50.x, r0.y
/*   54   */          mul r0._y__, r_abs[0].zzzz, c43.zzzz
              +       exp r0.x___, r0.x
/*   55   */          mulsc r0.x___, c43.w, r0.x
/*   56   */          mad r0.xyz_, r0.yyyy, r7.xyzz, r0.xxxx
/*   57   */          mul_sat r0.xyz_, r0.xyzz, c44.wwww
/*   58   */          max oC0, r0, r0
/*    9.1 */       cnop
