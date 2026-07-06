/*    0.0 */       exec
/*    5   */          tfetch2D r1.wxyz, r0.xy, tf0, FetchValidOnly=false
/*    6   */          serialize
                      sge r0.___w, r1.xxxx, c255.yyyy
/*    7   */          sge r1.x___, r1.wwww, c255.xxxx
/*    8   */          mad r0.__z_, -r1.xxxx, c254.zzzz, r1.wwww
/*    9   */          sge r1.___w, r0.zzzz, c254.yyyy
              +       subsc r0.__z_, c255.z, r0.w
/*   10   */          mul r1.___w, r0.zzzz, r1.wwww
/*    0.1 */       exec
/*   11   */          sne r2.x___, r1.wwww, c254.xxxx
              +       sgts r1.___w, -r_abs[0].x
/*   12   */          add r1.__z_, r2.xxxx, r1.zzzz
              +       subsc r1.x___, c255.z, r1.x
/*    1.0 */       loop i31, L8
                label L3
/*    1.1 */       exec    // PredicateClean=false
/*   13   */          setp_eq r0._, r1.w
/*   14   */     (p0) rcp r2.x___, c49.x
/*   15   */     (p0) rcp r2._y__, c48.x
/*   16   */     (p0) mul r2.xy__, r2.xyyy, c[20+aL].xyyy
/*   17   */     (p0) mad r2.xy__, r2.yxxx, c50.xxxx, r0.yxxx
/*   18   */     (p0) tfetch2D r2, r2.yx, tf0, FetchValidOnly=false
/*    2.0 */  (p0) exec
/*   19   */          serialize
                 (p0) sge r4._yz_, r2.zzww, c255.xxyy
              +  (p0) maxs r0._, r2.xx
/*   20   */     (p0) mad r1.___w, -r4.yyyy, c254.zzzz, r2.zzzz
/*   21   */     (p0) add_sat r4.___w, r0.wwww, r4.zzzz
              +  (p0) adds_prev r3.x___, -r1.y
/*   22   */     (p0) sge r4.x___, r_abs[3].xxxx, c255.wwww
/*   23   */     (p0) add r3, -r4.xzyw, c255.zzzz
/*   24   */     (p0) sge r1.___w, r1.wwww, c254.yyyy
              +  (p0) maxs r0._, r3.zz
/*    2.1 */  (p0) exec
/*   25   */     (p0) mad r1.___w, r3.yyyy, r1.wwww, r2.yyyy
/*   26   */     (p0) add r1.___w, r1.wwww, -r1.zzzz
              +  (p0) muls_prev r1.x___, r1.x
/*   27   */     (p0) add_sat r2.x___, r0.zzzz, r1.xxxx
/*   28   */     (p0) mul r1.___w, r1.xxxx, r_abs[1].wwww
              +  (p0) maxs r0._, r2.xx
/*   29   */     (p0) sge r2.x___, r1.wwww, c255.wwww
              +  (p0) muls_prev r1.___w, r4.x
/*   30   */     (p0) add r2.x___, -r1.wwww, r2.xxxx
              +  (p0) muls r2._y__, r3.wx
/*    3.0 */  (p0) exec
/*   31   */     (p0) mad r1.___w, r2.yyyy, r2.xxxx, r1.wwww
/*    3.1 */       endloop i31, L3
                label L8
/*    4.0 */       alloc colors
/*    4.1 */       exece
/*   32   */          max oC0.xyz1, r1.wwww, r1.wwww
