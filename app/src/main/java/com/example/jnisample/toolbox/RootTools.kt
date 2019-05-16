package com.example.jnisample.toolbox

/**
 * Created by Manne Öhlund on 2019-04-10.
 */
class RootTools {

    private var resultCallback: ((String) -> Unit)? = null
    private var errorCallback: ((String) -> Unit)? = null

    fun onResult(result: String) {
        this.resultCallback?.invoke(result)
    }

    fun onError(result: String) {
        this.errorCallback?.invoke(result)
    }

    fun init(): Int {
        return initShell()
    }

    fun exit(): Int {
        return exitShell()
    }

    fun interruptShell(): Int {
        return interrupt()
    }

    fun ls(path: String, result: ((String) -> Unit)?, error: ((String) -> Unit)?): Int {
        this.resultCallback = result
        this.errorCallback = error
        return ls(path)
    }

    fun find(path: String, name: String, result: ((String) -> Unit)?, error: ((String) -> Unit)?): Int {
        this.resultCallback = result
        this.errorCallback = error
        return find(path, name)
    }

    fun whoami(result: ((String) -> Unit)?, error: ((String) -> Unit)?): Int {
        this.resultCallback = result
        this.errorCallback = error
        return whoami()
    }

    fun exec(command: String, result: ((String) -> Unit)?, error: ((String) -> Unit)?): Int {
        this.resultCallback = result
        this.errorCallback = error
        return exec(command)
    }

    fun execute(command: String, result: ((String) -> Unit)?, error: ((String) -> Unit)?): Int {
        this.resultCallback = result
        this.errorCallback = error
        return execute(command)
    }

    private external fun initShell(): Int

    private external fun exitShell(): Int

    private external fun interrupt(): Int

    private external fun ls(path: String): Int

    private external fun find(path: String, name: String): Int

    private external fun whoami(): Int

    private external fun exec(command: String): Int

    private external fun execute(command: String): Int

    companion object {

        init {
            try {
                System.loadLibrary("roottools-lib")
            } catch (e: Exception) {
                println("ERROR: Unable to load roottools-lib.so")
                e.printStackTrace()
            }

        }
    }
}
