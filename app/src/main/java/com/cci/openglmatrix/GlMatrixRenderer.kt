package com.cci.openglmatrix

import android.os.Environment
import android.os.Process
import android.view.Surface
import androidx.annotation.WorkerThread
import java.io.File
import java.util.*
import java.util.concurrent.atomic.AtomicInteger

class GlMatrixRenderer {

    private val executor by lazy {
        SingleThreadHandlerExecutor(
            String.format(Locale.US, "GLRenderer-%03d", RENDERED_COUNT.incrementAndGet()),
            Process.THREAD_PRIORITY_DEFAULT
        )
    }


    fun drawTriangle(surface: Surface) {
        executor.execute {
            nativeMatrix(surface,
                File(Environment.getExternalStorageDirectory(),"container.jpg").absolutePath,
                File(Environment.getExternalStorageDirectory(),"awesomeface.png").absolutePath
            )

        }
    }


    companion object {
        private val RENDERED_COUNT = AtomicInteger(0)

        init {
            System.loadLibrary("opengl-renderer")
        }

        @WorkerThread
        @JvmStatic
        external fun nativeMatrix(surface: Surface,imageTexturePath:String,imageTexturePath1: String)

    }


}