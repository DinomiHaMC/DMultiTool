// SPDX-License-Identifier: GPL-3.0-only
package org.dmultitool.notifications;
import android.Manifest;
import android.app.Notification;
import android.bluetooth.*;
import android.content.*;
import android.content.pm.PackageManager;
import android.os.*;
import android.service.notification.*;
import java.nio.charset.StandardCharsets;
import java.util.*;
public class NotificationBridge extends NotificationListenerService {
    public static volatile NotificationBridge instance;
    public static volatile String status="Разрешите доступ к уведомлениям в настройках Android";
    private final Handler handler=new Handler(Looper.getMainLooper());
    private final ArrayDeque<String> pending=new ArrayDeque<>();
    private final LinkedHashMap<String,String> recent=new LinkedHashMap<>();
    private BluetoothGatt gatt;private BluetoothGattCharacteristic rx;
    private int mtu=23;private boolean writing=false;
    private final Runnable reconnect=()->refresh();
    @Override public void onCreate() { super.onCreate();instance=this; }
    @Override public void onListenerConnected() { status="Доступ к уведомлениям получен";refresh(); }
    @Override public void onListenerDisconnected() { close();status="Доступ к уведомлениям отключён"; }
    @Override public void onDestroy() { close();instance=null;super.onDestroy(); }
    public void refresh() { handler.post(()->{
        closeInternal();SharedPreferences prefs=getSharedPreferences("bridge",0);
        if(!prefs.getBoolean("enabled",false)) { status="Передача отключена";return; }
        if(Build.VERSION.SDK_INT>=31&&checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT)!=PackageManager.PERMISSION_GRANTED) { status="Нет разрешения Bluetooth";return; }
        BluetoothAdapter adapter=getSystemService(BluetoothManager.class).getAdapter();String address=prefs.getString("device","");
        if(adapter==null||!adapter.isEnabled()||address.isEmpty()) { status="Выберите сопряжённое устройство / включите Bluetooth";handler.postDelayed(reconnect,10000);return; }
        try { status="Подключение к DMultiTool";gatt=adapter.getRemoteDevice(address).connectGatt(this,false,callback,BluetoothDevice.TRANSPORT_LE); }
        catch(RuntimeException error) { status="Ошибка подключения: "+error.getMessage();handler.postDelayed(reconnect,10000); }
    }); }
    private void closeInternal() {
        handler.removeCallbacks(reconnect);rx=null;writing=false;mtu=23;
        if(gatt!=null) { try { gatt.disconnect();gatt.close(); }catch(SecurityException ignored) {}gatt=null; }
    }
    private void close() { handler.post(()->{closeInternal();pending.clear();}); }
    private final BluetoothGattCallback callback=new BluetoothGattCallback() {
        @Override public void onConnectionStateChange(BluetoothGatt connection,int result,int state) {
            handler.post(()->{
                if(connection!=gatt)return;
                if(state==BluetoothProfile.STATE_CONNECTED) { if(!connection.requestMtu(247))connection.discoverServices(); }
                else if(state==BluetoothProfile.STATE_DISCONNECTED) { closeInternal();status="Нет связи. Повтор через 10 секунд";handler.postDelayed(reconnect,10000); }
            });
        }
        @Override public void onMtuChanged(BluetoothGatt connection,int value,int result) { handler.post(()->{
            if(connection!=gatt)return;mtu=result==BluetoothGatt.GATT_SUCCESS?value:23;connection.discoverServices();
        }); }
        @Override public void onServicesDiscovered(BluetoothGatt connection,int result) { handler.post(()->{
            if(connection!=gatt)return;
            BluetoothGattService service=connection.getService(UUID.fromString("6e400001-b5a3-f393-e0a9-e50e24dcca9e"));
            rx=service==null?null:service.getCharacteristic(UUID.fromString("6e400002-b5a3-f393-e0a9-e50e24dcca9e"));
            if(result!=BluetoothGatt.GATT_SUCCESS||rx==null) { status="Нет сервиса DMultiTool: включите Receiver";return; }
            status="Подключено. Уведомления отправляются в DMultiTool";flush();
        }); }
        @Override public void onCharacteristicWrite(BluetoothGatt connection,BluetoothGattCharacteristic characteristic,int result) { handler.post(()->{
            if(connection!=gatt)return;writing=false;if(!pending.isEmpty())pending.removeFirst();
            status=result==BluetoothGatt.GATT_SUCCESS?"Уведомление отправлено":"Ошибка BLE: "+result;flush();
        }); }
    };
    public void send(String title,String body) { handler.post(()->{
        if(!getSharedPreferences("bridge",0).getBoolean("enabled",false))return;
        if(pending.size()>=20)pending.removeLast();pending.addLast(title+": "+body);flush();
    }); }
    @SuppressWarnings("deprecation") private void flush() {
        if(gatt==null||rx==null||writing||pending.isEmpty())return;
        String text=pending.peekFirst();int limit=Math.min(199,mtu-4);
        while(text.getBytes(StandardCharsets.UTF_8).length>limit&&!text.isEmpty())text=text.substring(0,text.offsetByCodePoints(text.length(),-1));
        byte[] encoded=text.getBytes(StandardCharsets.UTF_8),packet=new byte[encoded.length+1];packet[0]=0x30;System.arraycopy(encoded,0,packet,1,encoded.length);
        rx.setWriteType(BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT);rx.setValue(packet);writing=true;
        if(!gatt.writeCharacteristic(rx)) { writing=false;status="Не удалось отправить уведомление";handler.postDelayed(()->flush(),1000); }
    }
    @Override public void onNotificationPosted(StatusBarNotification item) {
        if(item.getPackageName().equals(getPackageName())||(item.getNotification().flags&Notification.FLAG_ONGOING_EVENT)!=0)return;
        Bundle extras=item.getNotification().extras;String title=String.valueOf(extras.getCharSequence(Notification.EXTRA_TITLE,"Notification"));
        String body=String.valueOf(extras.getCharSequence(Notification.EXTRA_TEXT,""));
        String key=item.getKey(),content=title+body;
        synchronized(recent) { if(content.equals(recent.get(key)))return;recent.put(key,content);if(recent.size()>32)recent.remove(recent.keySet().iterator().next()); }
        send(title,body);
    }
}
