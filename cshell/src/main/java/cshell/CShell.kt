package cshell

/**
 * Created by Manne Öhlund on 2019-04-10.
 */
class CShell {

    private var resultCallback: ((String) -> Unit) = {}

    private var errorCallback: ((String) -> Unit) = {}

    fun onResult(result: String) {
        this.resultCallback.invoke(result)
    }

    fun onError(result: String) {
        this.errorCallback.invoke(result)
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

    fun execute(command: String, result: ((String) -> Unit) = {}, error: ((String) -> Unit) = {}): Int {
        this.resultCallback = result
        this.errorCallback = error
        return execute(command)
    }

    private external fun initShell(): Int

    private external fun exitShell(): Int

    private external fun interrupt(): Int

    private external fun execute(command: String): Int

    companion object {

        init {
            try {
                System.loadLibrary("cshell-lib")
            } catch (e: Exception) {
                println("ERROR: Unable to load toolbox-lib.so")
                e.printStackTrace()
            }

        }
    }
}
