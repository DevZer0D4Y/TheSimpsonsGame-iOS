/*    0.0 */       exec
/*    3   */          vfetch_full r0._xy_, r0.x, vf0, DataFormat=FMT_32_32_FLOAT, Stride=9, Signed=true, NumFormat=integer, PrefetchCount=6
/*    4   */          vfetch_mini r2, Offset=2, DataFormat=FMT_32_32_32_32_FLOAT, Signed=true, NumFormat=integer
/*    5   */          vfetch_full r1.xyz_, r0.x, vf0, Offset=6, DataFormat=FMT_32_32_32_FLOAT, Stride=9, Signed=true, NumFormat=integer
/*    0.1 */       alloc position
/*    1.0 */       exec
/*    6   */          add r0.xy__, r0.zyyy, r0.zyyy
/*    7   */          add oPos._y11, -r0.xxxx, c255.xxxx
              +       subsc oPos.x___, -c255.x, -r0.y
/*    1.1 */       alloc interpolators
/*    2.0 */       exece
/*    8   */          maxs o1.x___, r1.zz
/*    9   */          mul o0, r2, r1.xyxy
/*    2.1 */       cnop
