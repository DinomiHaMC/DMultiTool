// SPDX-License-Identifier: GPL-3.0-only
package org.dmultitool.notifications;
import android.Manifest;
import android.app.*;
import android.bluetooth.*;
import android.content.*;
import android.content.pm.PackageManager;
import android.os.*;
import android.provider.Settings;
import android.service.notification.NotificationListenerService;
import android.widget.*;
import java.util.*;
public class MainActivity extends Activity {
    private final Handler handler = new Handler(Looper.getMainLooper());
    private TextView status;
    private final Runnable update = new Runnable() { public void run() { status.setText(NotificationBridge.status);handler.postDelayed(this,1000); } };
    private void refresh() {
        NotificationBridge bridge=NotificationBridge.instance;
        if(bridge!=null)bridge.refresh();
        else NotificationListenerService.requestRebind(new ComponentName(this,NotificationBridge.class));
    }
    private void button(LinearLayout layout,String name,Runnable callback) {
        Button button=new Button(this);button.setText(name);button.setOnClickListener(v->callback.run());layout.addView(button);
    }
    @Override public void onCreate(Bundle saved) {
        super.onCreate(saved);
        LinearLayout layout=new LinearLayout(this);layout.setOrientation(LinearLayout.VERTICAL);layout.setPadding(20,30,20,20);
        TextView title=new TextView(this);title.setText("DMultiTool: уведомления\n\n1. Включите Receiver в Utils → Notifications.\n2. Выполните сопряжение DMultiTool HID в настройках Bluetooth телефона.\n3. Разрешите доступ к уведомлениям и выберите устройство ниже.\n\nУведомления передаются только на выбранное устройство, без интернета.");layout.addView(title);
        button(layout,"Разрешить Bluetooth",()->{
            if(Build.VERSION.SDK_INT>=31)requestPermissions(new String[]{Manifest.permission.BLUETOOTH_CONNECT},1);
            else startActivity(new Intent(Settings.ACTION_BLUETOOTH_SETTINGS));
        });
        button(layout,"Доступ к уведомлениям",()->startActivity(new Intent(Settings.ACTION_NOTIFICATION_LISTENER_SETTINGS)));
        button(layout,"Выбрать устройство",()->choose());
        button(layout,"Отправить тест",()->{
            if(NotificationBridge.instance!=null)NotificationBridge.instance.send("DMultiTool","Тестовое уведомление");
            else Toast.makeText(this,"Сначала разрешите доступ к уведомлениям",Toast.LENGTH_LONG).show();
        });
        button(layout,"Отключить передачу",()->{
            getSharedPreferences("bridge",0).edit().putBoolean("enabled",false).apply();refresh();
        });
        status=new TextView(this);layout.addView(status);setContentView(layout);
    }
    private void choose() {
        if(Build.VERSION.SDK_INT>=31&&checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT)!=PackageManager.PERMISSION_GRANTED) {
            requestPermissions(new String[]{Manifest.permission.BLUETOOTH_CONNECT},1);return;
        }
        BluetoothManager manager=getSystemService(BluetoothManager.class);BluetoothAdapter adapter=manager.getAdapter();
        if(adapter==null||!adapter.isEnabled()) { startActivity(new Intent(Settings.ACTION_BLUETOOTH_SETTINGS));return; }
        ArrayList<BluetoothDevice> devices=new ArrayList<>();ArrayList<String> labels=new ArrayList<>();
        for(BluetoothDevice device:adapter.getBondedDevices()) { devices.add(device);labels.add(device.getName()+"\n"+device.getAddress()); }
        if(devices.isEmpty()) { Toast.makeText(this,"Сначала выполните сопряжение в настройках Bluetooth",Toast.LENGTH_LONG).show();return; }
        new AlertDialog.Builder(this).setTitle("DMultiTool").setItems(labels.toArray(new String[0]),(dialog,index)->{
            getSharedPreferences("bridge",0).edit().putString("device",devices.get(index).getAddress()).putBoolean("enabled",true).apply();refresh();
        }).show();
    }
    @Override public void onResume() { super.onResume();handler.post(update); }
    @Override public void onPause() { handler.removeCallbacks(update);super.onPause(); }
}
