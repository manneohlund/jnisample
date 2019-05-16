package com.example.jnisample

import android.os.Bundle
import android.support.v7.app.AppCompatActivity
import android.support.v7.widget.SearchView
import android.util.Log
import android.view.Menu
import android.view.MenuItem
import android.view.View
import android.widget.Toast
import com.example.jnisample.toolbox.RootTools
import com.example.jnisample.toolbox.commandList
import cshell.CShell
import kotlinx.android.synthetic.main.activity_main.*
import smartadapter.SmartRecyclerAdapter
import smartadapter.datatype.ViewEvent
import smartadapter.datatype.ViewEvent.ON_CLICK
import smartadapter.datatype.ViewEvent.ON_LONG_CLICK

class MainActivity : AppCompatActivity() {

    private val shell = CShell() // RootTools()
    lateinit var smartRecyclerAdapter: SmartRecyclerAdapter
    private lateinit var searchView: SearchView

    private lateinit var savedCommandsFile: String

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        savedCommandsFile = "${applicationContext.cacheDir}/saved-commands.txt"

        smartRecyclerAdapter = SmartRecyclerAdapter
            .empty()
            .map(String::class.java, ResultHolder::class.java)
            .map(ErrorString::class.java, ErrorResultHolder::class.java)
            .map(CommandString::class.java, CommandHolder::class.java)
            .map(SavedCommandString::class.java, SavedCommandHolder::class.java)
            .addViewEventListener(CommandHolder::class.java) { _, _, pos ->
                val cmd: CommandString = smartRecyclerAdapter.getItem(pos) as CommandString
                smartRecyclerAdapter.clear()
                execute(cmd.command)
            }
            .addViewEventListener(SavedCommandHolder::class.java, ViewEvent.ON_CLICK or ViewEvent.ON_LONG_CLICK) { view, actionId, position ->
                when (actionId) {
                    R.id.action_on_click -> {
                        (smartRecyclerAdapter.getItem(position) as? SavedCommandString)?.run {
                            Toast.makeText(applicationContext, command, Toast.LENGTH_LONG).show()
                            smartRecyclerAdapter.clear()
                            searchView.setQuery(command, false)
                            execute(command)
                        }
                    }
                    R.id.action_on_long_click -> {
                        Toast.makeText(this, "Remove $position", Toast.LENGTH_LONG).show()
                        smartRecyclerAdapter.removeItem(position)
                        shell.execute("sed -i '${position+1}d' $savedCommandsFile")
                    }
                }
            }
            .into(recyclerView)

        initShell()
        initCommands()
        initNativeLibEnvironments()
    }

    override fun onCreateOptionsMenu(menu: Menu?): Boolean {
        menuInflater.inflate(R.menu.toolbox_menu, menu);
        searchView = menu?.findItem(R.id.action_search)?.actionView as SearchView;
        initSearchView(searchView);
        return true;
    }

    override fun onOptionsItemSelected(item: MenuItem?): Boolean {
        item?.let {
            return when (item.getItemId()) {
                R.id.action_save -> {
                    shell.execute("echo \"${searchView.query}\" >> $savedCommandsFile")
                    true
                }
                R.id.action_clear -> {
                    initCommands()
                    true
                }
                R.id.action_init_native -> {
                    initNativeLibEnvironments()
                    true
                }
                R.id.action_interrupt -> {
                    smartRecyclerAdapter.clear()
                    Thread() {
                        run {
                            execute("sleep 4 && ls -lR /sdcard/ && ps -A | grep '.*'")
                        }
                    }.start();
                    interruptShell();
                    true
                }
                R.id.action_ls_system -> {
                    smartRecyclerAdapter.clear()
                    execute("ls -1 /system/bin")
                    true
                }
                R.id.action_ls_l_system -> {
                    smartRecyclerAdapter.clear()
                    execute("ls -l1 /system/bin")
                    true
                }
                R.id.action_ls_root -> {
                    doLs("/data/data/com.example.jnisample")
                    true
                }
                R.id.action_ls_sdcard -> {
                    doLs("/sdcard")
                    true
                }
                R.id.action_find_dotjpg -> {
                    doFind(".jpg")
                    true
                }
                R.id.action_find_jpg -> {
                    doFind("jpg")
                    true
                }
                R.id.action_find_screen -> {
                    doFind("Screen")
                    true
                }
                R.id.action_exit -> {
                    execute("exit")
                    true
                }
                else -> true
            }
        }
        return super.onOptionsItemSelected(item)
    }

    private fun initShell() {
        smartRecyclerAdapter.clear()
        val start = System.currentTimeMillis()
        val code = shell.init()
        val end = System.currentTimeMillis() - start;
        invalidateTitle(code, end)

        shell.execute("touch $savedCommandsFile");
    }

    private fun initCommands() {
        smartRecyclerAdapter.clear()
        for (cmd in commandList) {
            smartRecyclerAdapter.addItem(CommandString(cmd), false);
        }
        smartRecyclerAdapter.smartNotifyDataSetChanged()
    }

    private fun initSavedCommands() {
        smartRecyclerAdapter.clear()
        shell.execute(
            command = "cat $savedCommandsFile",
            result = { command ->
                smartRecyclerAdapter.addItem(SavedCommandString(command), false);
            },
            error = {
                smartRecyclerAdapter.addItem(ErrorString(it), false)
            })
        smartRecyclerAdapter.smartNotifyDataSetChanged()
    }

    private fun initSearchView(searchView: SearchView) {
        searchView.setOnQueryTextListener(object : SearchView.OnQueryTextListener {
            override fun onQueryTextSubmit(command: String?): Boolean {
                command?.run {
                    smartRecyclerAdapter.clear()
                    execute(command)
                }
                return false
            }

            override fun onQueryTextChange(command: String?): Boolean {
                when (command?.isEmpty()) {
                    true -> initSavedCommands()
                }
                return false
            }
        })

        searchView.setOnSearchClickListener {
            initSavedCommands()
        }

        searchView.setOnCloseListener {
            initCommands()
            false
        }
    }

    private fun initNativeLibEnvironments() {
        Toast.makeText(this, applicationContext.applicationInfo.nativeLibraryDir, Toast.LENGTH_LONG).show()
        Log.e("nativeLibraryDir", applicationContext.applicationInfo.nativeLibraryDir)

        val nativeLibDir = applicationContext.applicationInfo.nativeLibraryDir

        val cwhoami = "alias rswhoami='$nativeLibDir/libcwhoami-lib.so'"
        val cfind = "alias rsfind='$nativeLibDir/libcfind-lib.so'"
        execute(cwhoami)
        execute(cfind)

        val nativeLibraryDir = "alias nativeLibDir='$nativeLibDir'"
        val nativeLibraryDirToPath = "PATH=\$PATH:$nativeLibDir"
        execute(nativeLibraryDir)
        execute(nativeLibraryDirToPath)
    }

    fun exitShell() {
        smartRecyclerAdapter.clear()
        val start = System.currentTimeMillis()
        val code = shell.exit()
        val end = System.currentTimeMillis() - start;
        invalidateTitle(code, end)
    }

    fun interruptShell() {
        val start = System.currentTimeMillis()
        val code = shell.interruptShell()
        val end = System.currentTimeMillis() - start;
        invalidateTitle(code, end)
    }

    fun execute(command: String) {
        val start = System.currentTimeMillis()
        val code = shell.execute(
            command = command,
            result = {
                smartRecyclerAdapter.addItem(it, false)
            },
            error = {
                smartRecyclerAdapter.addItem(ErrorString(it), false)
            })
        val end = System.currentTimeMillis() - start;
        invalidateTitle(code, end)
    }

    fun doExec(command: String) {
        smartRecyclerAdapter.clear()
        val start = System.currentTimeMillis()
        val code = RootTools().exec(
            command = command,
            result = {
                Log.w(MainActivity::javaClass.name, it)
                smartRecyclerAdapter.addItem(it, false)
            },
            error = {
                smartRecyclerAdapter.addItem(ErrorString(it), false)
                Log.e(MainActivity::javaClass.name, it)
            })
        val end = System.currentTimeMillis() - start;
        invalidateTitle(code, end)
    }

    fun doLs(path: String) {
        smartRecyclerAdapter.clear()
        val start = System.currentTimeMillis()
        val code = RootTools().ls(
            path = path,
            result = {
                Log.w(MainActivity::javaClass.name, it)
                smartRecyclerAdapter.addItem(it, false)
            },
            error = {
                smartRecyclerAdapter.addItem(ErrorString(it), false)
                Log.e(MainActivity::javaClass.name, it)
            })
        val end = System.currentTimeMillis() - start;
        invalidateTitle(code, end)
    }

    fun doWhoami() {
        smartRecyclerAdapter.clear()
        val start = System.currentTimeMillis()
        val code = RootTools().whoami(
            result = {
                Log.w(MainActivity::javaClass.name, it)
                smartRecyclerAdapter.addItem(it, false)
            },
            error = {
                smartRecyclerAdapter.addItem(ErrorString(it), false)
                Log.e(MainActivity::javaClass.name, it)
            })
        val end = System.currentTimeMillis() - start;
        invalidateTitle(code, end)
    }

    fun doFind(word: String) {
        smartRecyclerAdapter.clear()
        val start = System.currentTimeMillis()
        val code = RootTools().find(
            path = "/sdcard",
            name = word,
            result = {
                Log.w(MainActivity::javaClass.name, it)
                smartRecyclerAdapter.addItem(it, false)
            },
            error = {
                smartRecyclerAdapter.addItem(ErrorString(it), false)
                Log.e(MainActivity::javaClass.name, it)
            })
        val end = System.currentTimeMillis() - start;
        invalidateTitle(code, end)
    }

    fun invalidateTitle(code: Int, end: Long) {
        runOnUiThread {
            //smartRecyclerAdapter.addItem("Exit code = $code\n" + "Execution time: $end millis")
            supportActionBar?.subtitle = "Exit code = $code\n" + "Execution time: $end millis"
            smartRecyclerAdapter.smartNotifyDataSetChanged()
        }
    }
}
