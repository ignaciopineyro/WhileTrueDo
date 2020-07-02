import numpy as np
from matplotlib import pyplot as plt

#Requisitos
#Vg = 9.6
Vg = 12.6
Vo = 5
Io = 2
#Vo = 20
#Io = 3
R = Vo/Io

#Parametros de diseño
fs = 100e3
Ts = 1/fs
M = Vo/Vg
n = 0.1
D_BB = M/(1+M)
Dprima_BB = 1-D_BB 
D_FB = M/(n+M)
Dprima_FB = 1-D_FB
R_L = 0.05
Delta_il = 2
Delta_vc = 2
C_oss1 = 50e-12
C_oss2 = 50e-12
C_j1 = 50e-12
C_j2 = 50e-12
Vr = 10
trr = 50e-9
Qrr = 200e-9
R_on1 = 5e-3
R_on2 = 5e-3
V_D1 = 0.3
V_D2 = 0.3
R_p = 0.05
R_s = 0.05
Lk = 10e-6

#Ejes
#D_BB = np.linspace(0.001, 0.999, 1000)
#Dprima_BB = 1-D_BB 
#D_FB = np.linspace(0.001, 0.999, 1000)
#Dprima_FB = 1-D_FB
#t = np.arange(0, Ts, 1000)
#Io = np.linspace(0, 4, 1000)
#fs = np.linspace(50e3, 500e3, 1000)

#Calculo de parametros
i_L_BB = Vo/(R*Dprima_BB)
#i_L_BB = Io/(Dprima_BB)
M_BB_ideal = D_BB/Dprima_BB
mu_BB = ((R*Dprima_BB**2))/(R_on1+R_L+R_on2+R*Dprima_BB**2)
M_BB = M_BB_ideal*mu_BB
L_BB = (Vg-(Vo/(R*Dprima_BB))*(R_on1+R_L+R_on2)*D_BB*Ts)/(2*Delta_il)
C_BB = (Vo*D_BB*Ts)/(R*2*Delta_vc)

i_L_FB = (n*Vo)/(R*Dprima_FB)
#i_L_FB = (n*Io)/Dprima_FB
M_FB_ideal = n*D_BB/Dprima_BB
mu_FB = (1-(Dprima_FB*V_D1)/(n*D_FB*Vg))*(1/(1+((D_FB*n**2)/Dprima_FB**2)*(R_p+R_on1)+R_s/(Dprima_FB*R)))
M_FB = M_FB_ideal*mu_FB
L_FB = (Vg - i_L_FB*(R_p+R_on1)*D_FB*Ts)/(2*Delta_il)
C_FB = (Vo*D_FB*Ts)/(R*2*Delta_vc)

#Tension y corriente en switches
ion_sw1_BB = i_L_BB
voff_sw1_BB = Vg
ion_sw2_BB = i_L_BB
voff_sw2_BB = Vg
ion_sw3_BB = i_L_BB
voff_sw3_BB = Vo
ion_sw4_BB = i_L_BB
voff_sw4_BB = Vo

ion_sw1_FB = i_L_FB
voff_sw1_FB = Vg+Vo/n
ion_sw2_FB = i_L_FB/n
voff_sw2_FB = (Vg+i_L_FB*(R_p + R_on1))/n
ion_sw3_FB = 0
voff_sw3_FB = 0
ion_sw4_FB = 0
voff_sw4_FB = 0

#Perdidas y rendimiento
Po = Io*Vo

P_Q1_BB = R_on1*i_L_BB**2
P_Q2_BB = R_on2*i_L_BB**2
P_D1_BB = V_D1*i_L_BB
P_D2_BB = V_D2*i_L_BB
P_SW1_BB = 0.5*C_oss1*Vo**2*fs
P_SW2_BB = 0.5*C_j1*Vr**2*fs
P_SW3_BB = 0.5*C_oss2*Vo**2*fs
P_SW4_BB = 0.5*C_j2*Vr**2*fs
P_RR1_BB = Vo*fs*(i_L_BB*trr+Qrr)
P_RR2_BB = Vo*fs*(i_L_BB*trr+Qrr)
Pcond_BB = P_Q1_BB + P_Q2_BB + P_D1_BB + P_D2_BB
Psw_BB = P_SW1_BB + P_SW2_BB + P_SW3_BB + P_SW4_BB + P_RR1_BB + P_RR2_BB
rend_BB = Po/(Po+Pcond_BB+Psw_BB)

P_Q1_FB = R_on1*i_L_FB**2
P_D1_FB = V_D1*i_L_FB
P_Lk_FB = 0.5*i_L_FB*Lk
P_SW1_FB = 0.5*C_oss1*Vo**2*fs
P_SW2_FB = 0.5*C_j1*Vr**2*fs
P_RR1_FB = Vg*fs*(i_L_BB*trr+Qrr)
Pcond_FB = P_Q1_FB + P_D1_FB
Psw_FB = P_SW1_FB + P_SW2_FB + P_RR1_FB
rend_FB = Po/(Po+Pcond_FB+Psw_FB)

#CURVAS

#M vs D
#plt.plot(D_BB, M_BB, label='Buck-Boost')
#plt.plot(D_FB, M_FB, label='Flyback')
#plt.xlabel('D')
#plt.ylabel('M(D)')
#plt.title('Vg = 12.6V, Vo = 5V @ 2A')

#i_L vs t
#plt.plot(t, i_L_BB, label='Buck-Boost')
#plt.plot(t, i_L_FB, label='Flyback')
#plt.xlabel('t')
#plt.ylabel('i_L')
#plt.title('Vg = 9.6V, Vo = 5V @ 2A')

#rend vs Io (RECORDAR: Poner i_L en funcion de Io en vez de Vo/R)
#plt.plot(Io, rend_BB, label='Buck-Boost')
#plt.plot(Io, rend_FB, label='Flyback')
#plt.xlabel('Io')
#plt.ylabel('rend(Io)')
#plt.title('Vg = 9.6V, Vo = 20V @ 3A')

#rend vs fs
#plt.plot(fs, rend_BB, label='Buck-Boost')
#plt.plot(fs, rend_FB, label='Flyback')
#plt.xlabel('fs')
#plt.ylabel('rend(fs)')
#plt.title('Vg = 12.6V, Vo = 5V @ 2A')

#plt.legend()
#plt.show()    
 