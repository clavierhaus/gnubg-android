package com.clavierhaus.gnubg.career

import android.content.Context
import android.util.Base64
import java.io.File
import java.security.AlgorithmParameters
import java.security.SecureRandom
import javax.crypto.Cipher
import javax.crypto.EncryptedPrivateKeyInfo
import javax.crypto.SecretKeyFactory
import javax.crypto.spec.PBEKeySpec
import javax.crypto.spec.PBEParameterSpec

/**
 * The career pass: the user's key in the section-7 formats
 * (docs/CRYPTOGRAPHY.md). Plain = PKCS#8 PEM. With passphrase = the
 * standard encrypted PKCS#8 envelope (PBES2: PBKDF2-HMAC-SHA256,
 * AES-256-CBC) -- the exact envelope openssl produces and decrypts,
 * proven in the build gate (JCA write -> openssl read, wrong passphrase
 * fails, JCA re-import).
 *
 * Never a silent fallback: if this device's providers cannot produce the
 * PBES2 envelope, the passphrase export FAILS and says so -- weaker crypto
 * is never substituted quietly.
 */
object CareerPass {

    private const val PBE_ALG = "PBEWithHmacSHA256AndAES_256"
    private const val ITERATIONS = 600_000

    /** The pass file bytes, plain: the PKCS#8 key, PEM-armored. Null if no key. */
    fun plainPem(ctx: Context): ByteArray? {
        val p8 = readKeyFile(ctx) ?: return null
        return pem("PRIVATE KEY", p8).toByteArray(Charsets.UTF_8)
    }

    /** The pass file bytes, passphrase-protected: encrypted PKCS#8 PEM.
     *  Null if no key or if this device cannot produce the standard envelope. */
    fun encryptedPem(ctx: Context, passphrase: CharArray): ByteArray? = runCatching {
        val p8 = readKeyFile(ctx) ?: return null
        val salt = ByteArray(16).also { SecureRandom().nextBytes(it) }
        val key = SecretKeyFactory.getInstance(PBE_ALG)
            .generateSecret(PBEKeySpec(passphrase))
        val c = Cipher.getInstance(PBE_ALG)
        c.init(Cipher.ENCRYPT_MODE, key, PBEParameterSpec(salt, ITERATIONS))
        val enc = c.doFinal(p8)
        // Re-wrap the parameters under the generic PBES2 name so the
        // EncryptedPrivateKeyInfo OID table accepts them (gate-proven).
        val pbes2 = AlgorithmParameters.getInstance("PBES2")
        pbes2.init(c.parameters.encoded)
        val epki = EncryptedPrivateKeyInfo(pbes2, enc)
        pem("ENCRYPTED PRIVATE KEY", epki.encoded).toByteArray(Charsets.UTF_8)
    }.getOrNull()

    private fun readKeyFile(ctx: Context): ByteArray? =
        File(ctx.filesDir, "career-key.p8").takeIf { it.exists() }?.readBytes()

    private fun pem(type: String, der: ByteArray): String {
        val b64 = Base64.encodeToString(der, Base64.NO_WRAP)
        return "-----BEGIN $type-----\n" +
            b64.chunked(64).joinToString("\n") +
            "\n-----END $type-----\n"
    }
}
