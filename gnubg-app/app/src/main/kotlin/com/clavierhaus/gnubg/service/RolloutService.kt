package com.clavierhaus.gnubg.service

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.Service
import android.content.Context
import android.content.Intent
import android.os.IBinder
import com.clavierhaus.gnubg.Engine
import kotlin.concurrent.thread

/**
 * Keeps the process alive while a rollout runs -- the maintainer's ruling
 * that continuation ships from the start: a rollout that dies when the
 * screen does would be exactly the cloud product's documented failure.
 * The computation itself runs in the app's own dispatcher; this service is
 * the lifecycle anchor plus an honest notification (candidate k/n, trials
 * done). It polls the same thread-safe status the UI polls and stops
 * itself the moment the rollout is over.
 */
class RolloutService : Service() {

    @Volatile private var watching = false

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        ensureChannel()
        startForeground(NOTE_ID, note("Rollout running"))
        if (!watching) {
            watching = true
            thread(name = "rollout-note") {
                val buf = IntArray(206)
                try {
                    Thread.sleep(1000)
                    while (watching) {
                        val n = Engine.rolloutStatus(buf)
                        if (n <= 0 || buf[0] == 0) break
                        val cur = if (buf[2] >= 0) buf[2] + 1 else buf[1]
                        val done = if (buf[2] >= 0) buf[6 + buf[2] * 25 + 10] else 0
                        notify(note("Candidate " + cur + "/" + buf[1] +
                                    " — " + done + "/" + buf[4] + " games"))
                        Thread.sleep(1000)
                    }
                } catch (_: InterruptedException) {
                } finally {
                    watching = false
                    stopSelf()
                }
            }
        }
        return START_NOT_STICKY
    }

    override fun onDestroy() {
        watching = false
        super.onDestroy()
    }

    private fun ensureChannel() {
        val nm = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
        if (nm.getNotificationChannel(CHANNEL) == null) {
            nm.createNotificationChannel(
                NotificationChannel(CHANNEL, "Rollouts", NotificationManager.IMPORTANCE_LOW)
            )
        }
    }

    private fun note(text: String): Notification =
        Notification.Builder(this, CHANNEL)
            .setContentTitle("CBG rollout")
            .setContentText(text)
            .setSmallIcon(android.R.drawable.stat_notify_sync)
            .setOngoing(true)
            .build()

    private fun notify(n: Notification) {
        (getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager)
            .notify(NOTE_ID, n)
    }

    companion object {
        private const val CHANNEL = "cbg_rollout"
        private const val NOTE_ID = 41

        fun start(ctx: Context) {
            ctx.startForegroundService(Intent(ctx, RolloutService::class.java))
        }

        fun stop(ctx: Context) {
            ctx.stopService(Intent(ctx, RolloutService::class.java))
        }
    }
}
