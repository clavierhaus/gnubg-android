import os

# 1. Update native-lib.c
native_c = "jni-bridge/src/native-lib.c"
with open(native_c, "r") as f: c_code = f.read()
if "Java_com_clavierhaus_gnubg_Engine_nextGame" not in c_code:
    with open(native_c, "a") as f:
        f.write("\nJNIEXPORT jintArray JNICALL\nJava_com_clavierhaus_gnubg_Engine_nextGame(JNIEnv *env, jobject thiz) {\n    pthread_mutex_lock(&gnubg_lock);\n    CommandNewGame(NULL);\n    jintArray result = pack_board(env, ms.anBoard);\n    pthread_mutex_unlock(&gnubg_lock);\n    return result;\n}\n")
if "Java_com_clavierhaus_gnubg_Engine_getMatchLength" not in c_code:
    with open(native_c, "a") as f:
        f.write("\nJNIEXPORT jint JNICALL\nJava_com_clavierhaus_gnubg_Engine_getMatchLength(JNIEnv *env, jobject thiz) {\n    return (jint)ms.nMatchTo;\n}\n")

# 2. Update Engine.kt
engine_kt = "gnubg-app/app/src/main/kotlin/com/clavierhaus/gnubg/Engine.kt"
with open(engine_kt, "r") as f: kt_code = f.read()
if "fun nextGame(" not in kt_code:
    kt_code = kt_code.replace("external fun newGame(matchLength: Int): IntArray", "external fun newGame(matchLength: Int): IntArray\n    external fun nextGame(): IntArray")
if "fun getMatchLength(" not in kt_code:
    kt_code = kt_code.replace("external fun getMatchScore(): IntArray", "external fun getMatchScore(): IntArray\n    external fun getMatchLength(): Int")
with open(engine_kt, "w") as f: f.write(kt_code)

# 3. Update BoardState.kt
bs_kt = "gnubg-app/app/src/main/kotlin/com/clavierhaus/gnubg/engine/BoardState.kt"
with open(bs_kt, "r") as f: bs_code = f.read()
if "val matchScore:" not in bs_code:
    bs_code = bs_code.replace("val boardHistory: List<IntArray> = emptyList(),", "val boardHistory: List<IntArray> = emptyList(),\n    val matchScore: IntArray = IntArray(2),\n    val matchLength: Int = 1,")
with open(bs_kt, "w") as f: f.write(bs_code)

# 4. Update GameViewModel.kt
gvm_kt = "gnubg-app/app/src/main/kotlin/com/clavierhaus/gnubg/engine/GameViewModel.kt"
with open(gvm_kt, "r") as f: gvm_code = f.read()
if "val matchLength = Engine.getMatchLength()" not in gvm_code:
    gvm_code = gvm_code.replace("val score     = Engine.getMatchScore()", "val score     = Engine.getMatchScore()\n        val matchLength = Engine.getMatchLength()")
if "matchScore = score" not in gvm_code:
    gvm_code = gvm_code.replace("board          = board", "matchScore     = score,\n            matchLength    = matchLength,\n            board          = board")
if "startNewGame(isNewMatch:" not in gvm_code:
    gvm_code = gvm_code.replace("fun startNewGame() {", "fun startNewGame(isNewMatch: Boolean = true) {")
    gvm_code = gvm_code.replace("Engine.newGame(_settings.value.matchLength)", "if (isNewMatch) Engine.newGame(_settings.value.matchLength) else Engine.nextGame()")
with open(gvm_kt, "w") as f: f.write(gvm_code)

# 5. Update Board.kt (Fix Doubling Cube Rendering)
board_kt = "gnubg-app/app/src/main/kotlin/com/clavierhaus/gnubg/ui/Board.kt"
with open(board_kt, "r") as f: b_code = f.read()

old_cube = """        // Cube drawn after bar checkers
        val cubeBarCX = ux(MID_X)
        val cubeBarCY = uy(TOT_H / 2f)
        val cubeSz = ux(BAR_W * 0.75f)
        drawCube(cubeBarCX - cubeSz / 2f, cubeBarCY - cubeSz / 2f, cubeSz, 64,
            p.cubeFace, p.cubeDot, p.cubeText)"""

new_cube = """        // Cube drawn after bar checkers
        val cubeBarCX = ux(MID_X)
        val cubeSz = ux(BAR_W * 0.75f)
        val cubeBarCY = when (gameState.cubeOwner) {
            0 -> uy(TOT_H - BRD_H) - cubeSz / 2f // Human owns it (bottom)
            1 -> uy(BRD_H) + cubeSz / 2f         // Engine owns it (top)
            else -> uy(TOT_H / 2f)               // Centred
        }
        val displayValue = if (gameState.cubeValue < 2) 64 else gameState.cubeValue

        if (gameState.phase == GamePhase.CUBE_OFFERED) {
            val bigCubeSz = ux(BAR_W * 2.5f)
            drawCube(ux(MID_X) - bigCubeSz / 2f, uy(TOT_H / 2f) - bigCubeSz / 2f, bigCubeSz, displayValue * 2, p.cubeFace, p.cubeDot, p.cubeText)
        } else {
            drawCube(cubeBarCX - cubeSz / 2f, cubeBarCY - cubeSz / 2f, cubeSz, displayValue, p.cubeFace, p.cubeDot, p.cubeText)
        }"""

if "gameState.cubeOwner" not in b_code and "drawCube(cubeBarCX" in b_code:
    b_code = b_code.replace(old_cube, new_cube)
    with open(board_kt, "w") as f: f.write(b_code)

print("OK Files safely patched by Python.")
