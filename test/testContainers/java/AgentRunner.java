import com.sun.tools.attach.VirtualMachine;

public class AgentRunner {

    /*
     * This class shows how to attach hotswap-agent.jar to a running JVM process and overload classes using "extraClasspath=" property via Hotswapper plugin.
     *
     * Lets assume that:
     *  args[0] contains pid of running JVM process or a runner class name we want to attach agent to
     *  "Usage: java -cp .:$JAVA_HOME/lib/tools.jar AgentRunner JVM_PID_OR_NAME"
     */
    public static void main(String[] args) {
        try {
            String pid = args[0];
            System.out.println("Main start");
            System.out.println(pid);
            System.out.println("Virtual Machine before attach");
            final VirtualMachine vm = VirtualMachine.attach(args[0]);
            System.out.println("Virtual Machine after attach");
            vm.loadAgentPath("/usr/local/scope/lib/libscope.so", null);
            System.out.println("Virtual Machine before detach");
            vm.detach();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}

// root@0dd6828b853d:/opt/appscope# javap -s com.sun.tools.attach.VirtualMachine
// Compiled from "VirtualMachine.java"
// public abstract class com.sun.tools.attach.VirtualMachine {
//   protected com.sun.tools.attach.VirtualMachine(com.sun.tools.attach.spi.AttachProvider, java.lang.String);
//     descriptor: (Lcom/sun/tools/attach/spi/AttachProvider;Ljava/lang/String;)V

//   public static java.util.List<com.sun.tools.attach.VirtualMachineDescriptor> list();
//     descriptor: ()Ljava/util/List;

//   public static com.sun.tools.attach.VirtualMachine attach(java.lang.String) throws com.sun.tools.attach.AttachNotSupportedException, java.io.IOException;
//     descriptor: (Ljava/lang/String;)Lcom/sun/tools/attach/VirtualMachine;

//   public static com.sun.tools.attach.VirtualMachine attach(com.sun.tools.attach.VirtualMachineDescriptor) throws com.sun.tools.attach.AttachNotSupportedException, java.io.IOException;
//     descriptor: (Lcom/sun/tools/attach/VirtualMachineDescriptor;)Lcom/sun/tools/attach/VirtualMachine;

//   public abstract void detach() throws java.io.IOException;
//     descriptor: ()V

//   public final com.sun.tools.attach.spi.AttachProvider provider();
//     descriptor: ()Lcom/sun/tools/attach/spi/AttachProvider;

//   public final java.lang.String id();
//     descriptor: ()Ljava/lang/String;

//   public abstract void loadAgentLibrary(java.lang.String, java.lang.String) throws com.sun.tools.attach.AgentLoadException, com.sun.tools.attach.AgentInitializationException, java.io.IOException;
//     descriptor: (Ljava/lang/String;Ljava/lang/String;)V

//   public void loadAgentLibrary(java.lang.String) throws com.sun.tools.attach.AgentLoadException, com.sun.tools.attach.AgentInitializationException, java.io.IOException;
//     descriptor: (Ljava/lang/String;)V

//   public abstract void loadAgentPath(java.lang.String, java.lang.String) throws com.sun.tools.attach.AgentLoadException, com.sun.tools.attach.AgentInitializationException, java.io.IOException;
//     descriptor: (Ljava/lang/String;Ljava/lang/String;)V

//   public void loadAgentPath(java.lang.String) throws com.sun.tools.attach.AgentLoadException, com.sun.tools.attach.AgentInitializationException, java.io.IOException;
//     descriptor: (Ljava/lang/String;)V

//   public abstract void loadAgent(java.lang.String, java.lang.String) throws com.sun.tools.attach.AgentLoadException, com.sun.tools.attach.AgentInitializationException, java.io.IOException;
//     descriptor: (Ljava/lang/String;Ljava/lang/String;)V

//   public void loadAgent(java.lang.String) throws com.sun.tools.attach.AgentLoadException, com.sun.tools.attach.AgentInitializationException, java.io.IOException;
//     descriptor: (Ljava/lang/String;)V

//   public abstract java.util.Properties getSystemProperties() throws java.io.IOException;
//     descriptor: ()Ljava/util/Properties;

//   public abstract java.util.Properties getAgentProperties() throws java.io.IOException;
//     descriptor: ()Ljava/util/Properties;

//   public abstract void startManagementAgent(java.util.Properties) throws java.io.IOException;
//     descriptor: (Ljava/util/Properties;)V

//   public abstract java.lang.String startLocalManagementAgent() throws java.io.IOException;
//     descriptor: ()Ljava/lang/String;

//   public int hashCode();
//     descriptor: ()I

//   public boolean equals(java.lang.Object);
//     descriptor: (Ljava/lang/Object;)Z

//   public java.lang.String toString();
//     descriptor: ()Ljava/lang/String;
// }
