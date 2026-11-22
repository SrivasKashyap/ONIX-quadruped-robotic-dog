import './globals.css'
import type { Metadata } from 'next'

export const metadata: Metadata = {
  title: 'ONYX - Railway Crack Intelligence Dashboard',
  description: 'Real-time track defect monitoring',
}

export default function RootLayout({
  children,
}: {
  children: React.ReactNode
}) {
  return (
    <html lang="en" className="dark">
      <body className="bg-[#090d16] text-gray-200 overflow-hidden h-screen w-screen m-0 p-0">
        {children}
      </body>
    </html>
  )
}