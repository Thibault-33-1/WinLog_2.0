# Désactiver l'UAC depuis le registre

# Vérifier si le script est exécuté en tant qu'administrateur
If (-Not ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator))
{
    Write-Warning "Ce script doit être exécuté en tant qu'administrateur!"
    Exit
}

# Chemin du registre pour désactiver l'UAC
$registryPath = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System"

# Désactiver l'UAC en définissant EnableLUA à 0
Set-ItemProperty -Path $registryPath -Name "EnableLUA" -Value 0

# Désactiver la virtualisation UAC
Set-ItemProperty -Path $registryPath -Name "PromptOnSecureDesktop" -Value 0

# Confirmation pour l'utilisateur
Write-Host "L'UAC a été désactivé via le registre. Un redémarrage est nécessaire pour que les modifications prennent effet."

# Optionnel : Demande à l'utilisateur de redémarrer le système manuellement
#Restart-Computer -Force