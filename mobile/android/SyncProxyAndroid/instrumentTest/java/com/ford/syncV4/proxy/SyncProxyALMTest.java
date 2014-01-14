package com.ford.syncV4.proxy;

import android.test.InstrumentationTestCase;

import com.ford.syncV4.exception.SyncException;
import com.ford.syncV4.protocol.ProtocolMessage;
import com.ford.syncV4.protocol.enums.MessageType;
import com.ford.syncV4.protocol.enums.SessionType;
import com.ford.syncV4.proxy.interfaces.IProxyListenerALM;
import com.ford.syncV4.proxy.rpc.SyncMsgVersion;
import com.ford.syncV4.proxy.rpc.enums.Language;
import com.ford.syncV4.proxy.rpc.enums.SyncInterfaceAvailability;
import com.ford.syncV4.syncConnection.SyncConnection;
import com.ford.syncV4.transport.TCPTransportConfig;
import com.ford.syncV4.transport.TransportType;

import org.mockito.ArgumentCaptor;
import org.mockito.Mockito;

import java.util.Arrays;

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

/**
 * Created by Andrew Batutin on 8/22/13.
 */
public class SyncProxyALMTest extends InstrumentationTestCase {

    public static final byte SESSION_ID = (byte) 48;
    private SyncProxyALM sut;

    public SyncProxyALMTest() {
    }

    @Override
    public void setUp() throws Exception {
        super.setUp();
        System.setProperty("dexmaker.dexcache", getInstrumentation().getTargetContext().getCacheDir().getPath());
        sut = getSyncProxyALM();
    }

    private SyncProxyALM getSyncProxyALM() throws SyncException {
        SyncMsgVersion syncMsgVersion = new SyncMsgVersion();
        syncMsgVersion.setMajorVersion(2);
        syncMsgVersion.setMinorVersion(2);
        TCPTransportConfig conf = mock(TCPTransportConfig.class);
        when(conf.getTransportType()).thenReturn(TransportType.TCP);
        final IProxyListenerALM listenerALM = mock(IProxyListenerALM.class);
        SyncProxyALM proxyALM = getSyncProxyALM(syncMsgVersion, conf, listenerALM);
        return proxyALM;
    }

    private SyncProxyALM getSyncProxyALM(final SyncMsgVersion syncMsgVersion, final TCPTransportConfig conf, final IProxyListenerALM listenerALM) throws SyncException {
        return new SyncProxyALM(listenerALM,
                                /*sync proxy configuration resources*/null,
                                /*enable advanced lifecycle management true,*/
                "appName",
                                /*ngn media app*/null,
                                /*vr synonyms*/null,
                                /*is media app*/true,
                                /*App type*/null,
                syncMsgVersion,
                                /*language desired*/Language.EN_US,
                                /*HMI Display Language Desired*/Language.EN_US,
                                /*App ID*/"8675308",
                                /*autoActivateID*/null,
                                /*callbackToUIThre1ad*/ false,
                                /*preRegister*/ false,
                2,
                conf) {


            @Override
            protected void initializeProxy() throws SyncException {
                // Reset all of the flags and state variables
                _haveReceivedFirstNonNoneHMILevel = false;
                _haveReceivedFirstFocusLevel = false;
                _haveReceivedFirstFocusLevelFull = false;
                _syncIntefaceAvailablity = SyncInterfaceAvailability.SYNC_INTERFACE_UNAVAILABLE;

                // Setup SyncConnection
                synchronized (CONNECTION_REFERENCE_LOCK) {
                    if (_syncConnection != null) {
                        _syncConnection.closeConnection(_rpcSessionID, false);
                        _syncConnection = null;
                    }
                    _syncConnection = mock(SyncConnection.class);
                    when(_syncConnection.getIsConnected()).thenReturn(true);
                }
                synchronized (CONNECTION_REFERENCE_LOCK) {
                    if (_syncConnection != null) {
                        _syncConnection.startTransport();
                    }
                }
            }
        };
    }

    public void testSyncProxyBaseShouldBeCreated() throws Exception {
        SyncProxyALM syncProxy = getSyncProxyALM();
        assertNotNull("Sync Proxy should not be null", syncProxy);
    }

    public void testSendVideoFrameShouldAddFrameToQueue() throws Exception {
        final byte[] testRTP = new byte[10];
        try {
            boolean result = sut.sendVideoFrame(testRTP);
            assertTrue("Failed to queue message", result);
        } catch (SyncException e) {
            assertNull("Should not get here", e);
        }
    }

    public void testVideoFrameShouldGetRightSessionIDAfterOnProtocolSessionStarted() throws Exception {
        final byte[] testRTP = new byte[10];
        sut.getInterfaceBroker().onProtocolSessionStarted(SessionType.Mobile_Nav, (byte) 23, (byte) 2, "");
        ProtocolMessage result = sut.createMobileNavSessionProtocolMessage(testRTP);
        assertTrue(Arrays.equals(testRTP, result.getData()));
        assertEquals(sut.getMobileNavSessionID(), result.getSessionID());
        assertEquals(MessageType.VIDEO, result.getMessageType());
        assertEquals(sut.getWiProVersion(), result.getVersion());
        assertEquals(SessionType.Mobile_Nav, result.getSessionType());
    }

    public void testMobileNavProtocolMessageCreation() throws Exception {
        final byte[] testRTP = new byte[10];
        ProtocolMessage message = sut.createMobileNavSessionProtocolMessage(testRTP);
        assertTrue(Arrays.equals(testRTP, message.getData()));
        assertEquals(sut.getMobileNavSessionID(), message.getSessionID());
        assertEquals(MessageType.VIDEO, message.getMessageType());
        assertEquals(sut.getWiProVersion(), message.getVersion());
        assertEquals(SessionType.Mobile_Nav, message.getSessionType());
    }

    public void testOnMobileNavSessionStarted() throws Exception {
        SyncMsgVersion syncMsgVersion = new SyncMsgVersion();
        syncMsgVersion.setMajorVersion(2);
        syncMsgVersion.setMinorVersion(2);
        TCPTransportConfig conf = mock(TCPTransportConfig.class);
        when(conf.getTransportType()).thenReturn(TransportType.TCP);
        final IProxyListenerALM listenerALM = mock(IProxyListenerALM.class);

        SyncProxyALM proxyALM = new SyncProxyALM(listenerALM,
                                /*sync proxy configuration resources*/null,
                                /*enable advanced lifecycle management true,*/
                "appName",
                                /*ngn media app*/null,
                                /*vr synonyms*/null,
                                /*is media app*/true,
                                /*app type*/null,
                syncMsgVersion,
                                /*language desired*/Language.EN_US,
                                /*HMI Display Language Desired*/Language.EN_US,
                                /*App ID*/"8675308",
                                /*autoActivateID*/null,
                                /*callbackToUIThre1ad*/ false,
                                /*preRegister*/ false,
                2,
                conf) {


            @Override
            protected void initializeProxy() throws SyncException {
                // Reset all of the flags and state variables
                _haveReceivedFirstNonNoneHMILevel = false;
                _haveReceivedFirstFocusLevel = false;
                _haveReceivedFirstFocusLevelFull = false;
                _syncIntefaceAvailablity = SyncInterfaceAvailability.SYNC_INTERFACE_UNAVAILABLE;

                // Setup SyncConnection
                synchronized (CONNECTION_REFERENCE_LOCK) {
                    if (_syncConnection != null) {
                        _syncConnection.closeConnection(_rpcSessionID, false);
                        _syncConnection = null;
                    }
                    _syncConnection = mock(SyncConnection.class);
                    when(_syncConnection.getIsConnected()).thenReturn(true);
                }
                synchronized (CONNECTION_REFERENCE_LOCK) {
                    if (_syncConnection != null) {
                        _syncConnection.startTransport();
                    }
                }
            }

            @Override
            protected void startMobileNavSession(byte sessionID, String correlationID) {
                super.startMobileNavSession(sessionID, correlationID);
                assertEquals("Session ID should be equal", _mobileNavSessionID, (byte) 48);
            }
        };
        proxyALM.getInterfaceBroker().onProtocolSessionStarted(SessionType.Mobile_Nav, (byte) 48, (byte) 2, "");
    }

    public void testReceivedMobileNavSessionIncomingMessage() throws Exception {
        SyncMsgVersion syncMsgVersion = new SyncMsgVersion();
        syncMsgVersion.setMajorVersion(2);
        syncMsgVersion.setMinorVersion(2);
        TCPTransportConfig conf = mock(TCPTransportConfig.class);
        when(conf.getTransportType()).thenReturn(TransportType.TCP);
        final IProxyListenerALM listenerALM = mock(IProxyListenerALM.class);
        SyncProxyALM proxyALM = new SyncProxyALM(listenerALM,
                                /*sync proxy configuration resources*/null,
                                /*enable advanced lifecycle management true,*/
                "appName",
                                /*ngn media app*/null,
                                /*vr synonyms*/null,
                                /*is media app*/true,
                                /*app type*/null,
                syncMsgVersion,
                                /*language desired*/Language.EN_US,
                                /*HMI Display Language Desired*/Language.EN_US,
                                /*App ID*/"8675308",
                                /*autoActivateID*/null,
                                /*callbackToUIThre1ad*/ false,
                                /*preRegister*/ false,
                2,
                conf) {


            @Override
            protected void initializeProxy() throws SyncException {
                // Reset all of the flags and state variables
                _haveReceivedFirstNonNoneHMILevel = false;
                _haveReceivedFirstFocusLevel = false;
                _haveReceivedFirstFocusLevelFull = false;
                _syncIntefaceAvailablity = SyncInterfaceAvailability.SYNC_INTERFACE_UNAVAILABLE;

                // Setup SyncConnection
                synchronized (CONNECTION_REFERENCE_LOCK) {
                    if (_syncConnection != null) {
                        _syncConnection.closeConnection(_rpcSessionID, false);
                        _syncConnection = null;
                    }
                    _syncConnection = mock(SyncConnection.class);
                    when(_syncConnection.getIsConnected()).thenReturn(true);
                }
                synchronized (CONNECTION_REFERENCE_LOCK) {
                    if (_syncConnection != null) {
                        _syncConnection.startTransport();
                    }
                }
            }

            @Override
            protected void handleMobileNavMessage(ProtocolMessage message) {
                super.handleMobileNavMessage(message);
                assertEquals(message.getSessionType(), SessionType.Mobile_Nav);
                assertEquals(message.getVersion(), (byte) 2);
                assertTrue(message.getSessionID() == (byte) 48);
            }
        };
        ProtocolMessage message = new ProtocolMessage();
        message.setVersion((byte) 2);
        message.setSessionID((byte) 48);
        message.setSessionType(SessionType.Mobile_Nav);
        proxyALM.dispatchIncomingMessage(message);
    }

    public void testSyncProxyBaseEndSessionCallbackTest() throws Exception {
        SyncMsgVersion syncMsgVersion = new SyncMsgVersion();
        syncMsgVersion.setMajorVersion(2);
        syncMsgVersion.setMinorVersion(2);
        TCPTransportConfig conf = mock(TCPTransportConfig.class);
        final IProxyListenerALM listenerALM = mock(IProxyListenerALM.class);
        SyncProxyALM proxyALM = new SyncProxyALM(listenerALM,
                                /*sync proxy configuration resources*/null,
                                /*enable advanced lifecycle management true,*/
                "appName",
                                /*ngn media app*/null,
                                /*vr synonyms*/null,
                                /*is media app*/true,
                                /*app type*/null,
                syncMsgVersion,
                                /*language desired*/Language.EN_US,
                                /*HMI Display Language Desired*/Language.EN_US,
                                /*App ID*/"8675308",
                                /*autoActivateID*/null,
                                /*callbackToUIThre1ad*/ false,
                                /*preRegister*/ false,
                2,
                conf) {


            @Override
            protected void initializeProxy() throws SyncException {
                // Reset all of the flags and state variables
                _haveReceivedFirstNonNoneHMILevel = false;
                _haveReceivedFirstFocusLevel = false;
                _haveReceivedFirstFocusLevelFull = false;
                _syncIntefaceAvailablity = SyncInterfaceAvailability.SYNC_INTERFACE_UNAVAILABLE;

                // Setup SyncConnection
                synchronized (CONNECTION_REFERENCE_LOCK) {
                    if (_syncConnection != null) {
                        _syncConnection.closeConnection(_rpcSessionID, false);
                        _syncConnection = null;
                    }
                    _syncConnection = mock(SyncConnection.class);
                    when(_syncConnection.getIsConnected()).thenReturn(true);
                }
                synchronized (CONNECTION_REFERENCE_LOCK) {
                    if (_syncConnection != null) {
                        _syncConnection.startTransport();
                    }
                }
            }

        };
        ArgumentCaptor<SessionType> sessionTypeCaptor = ArgumentCaptor.forClass(SessionType.class);
        ArgumentCaptor<Byte> sessionIdCaptor = ArgumentCaptor.forClass(byte.class);
        ArgumentCaptor<String> correlationIdCaptor = ArgumentCaptor.forClass(String.class);
        proxyALM.handleEndSessionAck(SessionType.RPC, SESSION_ID, "correlationID");
        Mockito.verify(listenerALM).onProtocolSessionEnded(sessionTypeCaptor.capture(), sessionIdCaptor.capture(), correlationIdCaptor.capture());
        assertEquals(SessionType.RPC, sessionTypeCaptor.getValue());
        assertEquals(SESSION_ID, sessionIdCaptor.getValue().byteValue());
        assertEquals("correlationID", correlationIdCaptor.getValue());
    }
}
